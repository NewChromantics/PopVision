#include "KinectAzure.h"
#include <k4abt.h>
#include <k4a/k4a.h>
#include "SoyDebug.h"
#include "HeapArray.hpp"
#include "MagicEnum/include/magic_enum.hpp"
#include "SoyRuntimeLibrary.h"
#include "SoyThread.h"

#if !defined(ENABLE_KINECTAZURE)
#error Expected ENABLE_KINECTAZURE to be defined
#endif

namespace Kinect
{
	void		IsOkay(k4a_result_t Error, const char* Context);
	void		IsOkay(k4a_wait_result_t Error, const char* Context);
	void		InitDebugHandler();
	void		LoadDll();

	float		GetScore(k4abt_joint_confidence_level_t Confidence);
	void		GetLabels(ArrayBridge<std::string>& Labels);

	//	remap enum to allow constexpr for magic_enum
	namespace JointLabel
	{
		enum TYPE
		{
			LeftEye = K4ABT_JOINT_EYE_LEFT,
			LeftEar = K4ABT_JOINT_EAR_LEFT,
			RightEye = K4ABT_JOINT_EYE_RIGHT,
			RightEar = K4ABT_JOINT_EAR_RIGHT,
			Head = K4ABT_JOINT_HEAD,
			Neck = K4ABT_JOINT_NECK,
			Nose = K4ABT_JOINT_NOSE,
			Chest = K4ABT_JOINT_SPINE_CHEST,
			Navel = K4ABT_JOINT_SPINE_NAVEL,
			Pelvis = K4ABT_JOINT_PELVIS,

			LeftClavicle = K4ABT_JOINT_CLAVICLE_LEFT,
			LeftShoulder = K4ABT_JOINT_SHOULDER_LEFT,
			LeftElbow = K4ABT_JOINT_ELBOW_LEFT,
			LeftWrist = K4ABT_JOINT_WRIST_LEFT,
			LeftHand = K4ABT_JOINT_HAND_LEFT,
			LeftFinger = K4ABT_JOINT_HANDTIP_LEFT,
			LeftThumb = K4ABT_JOINT_THUMB_LEFT,
			LeftHip = K4ABT_JOINT_HIP_LEFT,
			LeftKnee = K4ABT_JOINT_KNEE_LEFT,
			LeftAnkle = K4ABT_JOINT_ANKLE_LEFT,
			LeftFoot = K4ABT_JOINT_FOOT_LEFT,

			RightClavicle = K4ABT_JOINT_CLAVICLE_RIGHT,
			RightShoulder = K4ABT_JOINT_SHOULDER_RIGHT,
			RightElbow = K4ABT_JOINT_ELBOW_RIGHT,
			RightWrist = K4ABT_JOINT_WRIST_RIGHT,
			RightHand = K4ABT_JOINT_HAND_RIGHT,
			RightFinger = K4ABT_JOINT_HANDTIP_RIGHT,
			RightThumb = K4ABT_JOINT_THUMB_RIGHT,
			RightHip = K4ABT_JOINT_HIP_RIGHT,
			RightKnee = K4ABT_JOINT_KNEE_RIGHT,
			RightAnkle = K4ABT_JOINT_ANKLE_RIGHT,
			RightFoot = K4ABT_JOINT_FOOT_RIGHT,


			//Count = K4ABT_JOINT_COUNT
		};
	}
}

namespace CoreMl
{
	class TWorldObjectList;
	class TKinectAzureDevice;
	class TKinectAzureSkeletonReader;
}

class CoreMl::TWorldObjectList
{
public:
	Array<TWorldObject>	mObjects;
};


//	a device just holds low-level handle
class CoreMl::TKinectAzureDevice
{
public:
	TKinectAzureDevice(size_t DeviceIndex);
	~TKinectAzureDevice();

private:
	void				Shutdown();

public:
	k4a_device_t		mDevice = nullptr;
	k4abt_tracker_t		mTracker = nullptr;
};


//	reader is a thread reading skeletons from a device
class CoreMl::TKinectAzureSkeletonReader : public SoyThread
{
public:
	//	gr: because of the current use of the API, we have an option to keep alive here
	//		todo: pure RAII, but need to fix PopEngine first
	TKinectAzureSkeletonReader(size_t DeviceIndex,bool KeepAlive);

	TWorldObjectList	PopFrame(bool Blocking);

private:
	void				Iteration(int32_t TimeoutMs);
	virtual void		Thread() override;
	void				PushFrame(const TWorldObjectList& Objects);
	void				PushFrame(const k4abt_frame_t Frame);

	void				Open();

public:
	Soy::TSemaphore		mOnNewFrameSemaphore;
	std::shared_ptr<TKinectAzureDevice>	mDevice;

	std::mutex			mLastFrameLock;
	TWorldObjectList	mLastFrame;
	bool				mKeepAlive = false;
	size_t				mDeviceIndex = 0;
};



void Kinect::LoadDll()
{
	static bool DllLoaded = false;

	//	load the lazy-load libraries
	if (DllLoaded)
		return;

	//	we should just try, because if the parent process has loaded it, this
	//	will just load
	Soy::TRuntimeLibrary DllK4a("k4a.dll");
	Soy::TRuntimeLibrary DllK4abt("k4abt.dll");

	DllLoaded = true;
}

void Kinect::InitDebugHandler()
{
	LoadDll();

	auto OnDebug = [](void* Context, k4a_log_level_t Level, const char* Filename, const int Line, const char* Message)
	{
		std::Debug << "Kinect[" << magic_enum::enum_name(Level) << "] " << Filename << "(" << Line << "): " << Message << std::endl;
	};

	void* Context = nullptr;
	//auto DebugLevel = K4A_LOG_LEVEL_TRACE;
	auto DebugLevel = K4A_LOG_LEVEL_WARNING;
	auto Result = k4a_set_debug_message_handler(OnDebug, Context, DebugLevel);
	IsOkay(Result, "k4a_set_debug_message_handler");

	//	set these env vars for extra logging
	//	https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/709
	//Platform::SetEnvVar("K4ABT_ENABLE_LOG_TO_A_FILE", "k4abt.log");
	//Platform::SetEnvVar("K4ABT_LOG_LEVEL", "i");	//	or W
}


CoreMl::TKinectAzureDevice::TKinectAzureDevice(size_t DeviceIndex)
{
	//	this fails the second time if we didn't close properly (app still has exclusive access)
	//	so make sure we shutdown if we fail
	auto Error = k4a_device_open(DeviceIndex, &mDevice);
	Kinect::IsOkay(Error, "k4a_device_open");

	try
	{
		k4a_device_configuration_t DeviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
		DeviceConfig.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
		DeviceConfig.color_resolution = K4A_COLOR_RESOLUTION_OFF;
		Error = k4a_device_start_cameras(mDevice, &DeviceConfig);
		Kinect::IsOkay(Error, "k4a_device_start_cameras");
		
		k4a_calibration_t Calibration;
		Error = k4a_device_get_calibration(mDevice, DeviceConfig.depth_mode, DeviceConfig.color_resolution, &Calibration);
		Kinect::IsOkay(Error, "k4a_device_get_calibration");

		//	gr: as this takes a while to load, stop the cameras, then restart when needed
		//k4a_device_stop_cameras(mDevice);
		
		k4abt_tracker_configuration_t TrackerConfig = K4ABT_TRACKER_CONFIG_DEFAULT;
		Error = k4abt_tracker_create(&Calibration, TrackerConfig, &mTracker);
		Kinect::IsOkay(Error, "k4abt_tracker_create");

		k4abt_tracker_set_temporal_smoothing(mTracker, 0.f);
	}
	catch (std::exception& e)
	{
		Shutdown();
		throw;
	}
}

CoreMl::TKinectAzureDevice::~TKinectAzureDevice()
{
	try
	{
		Shutdown();
	}
	catch (std::exception& e)
	{
		std::Debug << "Error shutting down k4a device " << e.what() << std::endl;
	}
};

void CoreMl::TKinectAzureDevice::Shutdown()
{
	k4a_device_stop_cameras(mDevice);
	k4abt_tracker_shutdown(mTracker);
	k4abt_tracker_destroy(mTracker);
	k4a_device_close(mDevice);
}


CoreMl::TKinectAzure::TKinectAzure()
{
	Kinect::LoadDll();
	Kinect::InitDebugHandler();

	auto DeviceCount = k4a_device_get_installed_count();
	std::Debug << "KinectDevice count: " << DeviceCount << std::endl;

	auto DeviceIndex = 0;
	//	todo: remove keep alive when PopEngine/CAPI is fixed
	auto KeepAlive = true;	//	keep reopening the device in the reader
	mNative.reset( new TKinectAzureSkeletonReader(DeviceIndex, KeepAlive) );
}


float Kinect::GetScore(k4abt_joint_confidence_level_t Confidence)
{
	switch (Confidence)
	{
	case K4ABT_JOINT_CONFIDENCE_NONE:	return 0;
	case K4ABT_JOINT_CONFIDENCE_LOW:	return 0.33f;
	case K4ABT_JOINT_CONFIDENCE_MEDIUM:	return 0.66f;
	case K4ABT_JOINT_CONFIDENCE_HIGH:	return 1;
	default:	return -1;
	}
}

void CoreMl::TKinectAzure::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TWorldObject&)>& EnumObject)
{
	auto Blocking = true;
	auto Frame = mNative->PopFrame(Blocking);

	for (auto i = 0; i < Frame.mObjects.GetSize(); i++)
	{
		auto& Object = Frame.mObjects[i];
		EnumObject(Object);
	}
}




void CoreMl::TKinectAzure::GetLabels(ArrayBridge<std::string>&& Labels)
{
	Kinect::GetLabels(Labels);
}

void Kinect::GetLabels(ArrayBridge<std::string>& Labels)
{
	auto Names = magic_enum::enum_names<Kinect::JointLabel::TYPE>();
	for (auto&& Name : Names)
	{
		Labels.PushBack(std::string(Name));
	}
}


void Kinect::IsOkay(k4a_result_t Error, const char* Context)
{
	if (Error == K4A_RESULT_SUCCEEDED)
		return;

	std::stringstream ErrorString;
	ErrorString << "K4A error " << magic_enum::enum_name(Error) << " in " << Context;
	throw Soy::AssertException(ErrorString);
}


void Kinect::IsOkay(k4a_wait_result_t Error, const char* Context)
{
	if (Error == K4A_WAIT_RESULT_SUCCEEDED)
		return;

	std::stringstream ErrorString;
	ErrorString << "K4A wait error " << magic_enum::enum_name(Error) << " in " << Context;
	throw Soy::AssertException(ErrorString);
}


CoreMl::TKinectAzureSkeletonReader::TKinectAzureSkeletonReader(size_t DeviceIndex,bool KeepAlive) :
	SoyThread		("TKinectAzureSkeletonReader"),
	mDeviceIndex	( DeviceIndex ),
	mKeepAlive		( KeepAlive )
{
	//	try to open once to try and throw at construction (needed for non-keepalive anyway)
	Open();
	Start();
}


void CoreMl::TKinectAzureSkeletonReader::Open()
{
	if (mDevice)
		return;

	mDevice.reset();
	mDevice.reset(new TKinectAzureDevice(mDeviceIndex));
}

void CoreMl::TKinectAzureSkeletonReader::Thread()
{
	//	gr: have a timeout, so we can abort if the thread is stopped
	//		5 secs is a good indication something has gone wrong I think...
	try
	{
		Open();

		int32_t Timeout = 5000;
		Iteration(Timeout);
	}
	catch (std::exception& e)
	{
		//	gr: if we get this, we should restart the capture/acquire device
		std::Debug << "Exception in TKinectAzureSkeletonReader loop: " << e.what() << std::endl;
		
		if (mKeepAlive)
		{
			//	close device and let next iteration reopen
			mDevice.reset();
		}
		else
		{
			this->mOnNewFrameSemaphore.OnFailed(e.what());
			throw;
		}
	}
}


void CoreMl::TKinectAzureSkeletonReader::Iteration(int32_t TimeoutMs)
{
	if (!mDevice)
		throw Soy::AssertException("TKinectAzureSkeletonReader::Iteration null device");

	auto& mTracker = mDevice->mTracker;
	auto& Device = mDevice->mDevice;

	k4a_capture_t Capture = nullptr;

	//	gr: if we wait(eg breaked) for a long time, this just keeps timing out
	//		like the device has gone to sleep? need to startvideo again?
	//		or abort if it keeps happening and let high level restart?
	//	get capture of one frame (if we don't refresh this, the skeleton doesn't move)

	//	docs:
	/*	* This function returns an error when an internal problem is encountered; such as loss of the USB connection, inability
		* to allocate enough memory, and other unexpected issues.Any error returned by this function signals the end of
		* streaming data, and caller should stop the stream using k4a_device_stop_cameras().
		*/
	auto WaitError = k4a_device_get_capture(Device, &Capture, TimeoutMs);
	/*	gr: if we disconnect the device, we just get timeout, so we can't tell the difference
	//		if timeout is high then we can assume its dead. so throw and let the system try and reconnect
	if (WaitError == K4A_WAIT_RESULT_TIMEOUT)
	{
		std::Debug << "Kinect get-capture timeout (" << TimeoutMs << "ms)" << std::endl;
		return;
	}
	*/
	Kinect::IsOkay(WaitError, "k4a_device_get_capture");

	auto FreeCapture = [&]()
	{
		k4a_capture_release(Capture);
	};

	try
	{
		//	gr: apparently this enqueue should never timeout?
		//	https://github.com/microsoft/Azure-Kinect-Samples/blob/master/body-tracking-samples/simple_cpp_sample/main.cpp#L69
		//	request skeleton
		auto WaitError = k4abt_tracker_enqueue_capture(mTracker, Capture, TimeoutMs);
		Kinect::IsOkay(WaitError, "k4abt_tracker_enqueue_capture");

		//	pop [skeleton] frame
		//	gr: should this be infinite?
		k4abt_frame_t Frame = nullptr;
		auto WaitResult = k4abt_tracker_pop_result(mTracker, &Frame, TimeoutMs);
		Kinect::IsOkay(WaitResult, "k4abt_tracker_pop_result");

		//	extract skeletons
		PushFrame(Frame);

		//	cleanup
		k4abt_frame_release(Frame);
		FreeCapture();
	}
	catch (...)
	{
		FreeCapture();
		throw;
	}
}

void CoreMl::TKinectAzureSkeletonReader::PushFrame(const TWorldObjectList& Objects)
{
	{
		std::lock_guard<std::mutex> Lock(mLastFrameLock);
		mLastFrame = Objects;
	}

	//	notify new frame
	mOnNewFrameSemaphore.OnCompleted();
}


void CoreMl::TKinectAzureSkeletonReader::PushFrame(const k4abt_frame_t Frame)
{
	//	hacky optimisation before we switch to string_view everywhere
	static BufferArray<std::string, K4ABT_JOINT_COUNT> JointLabels;
	if (JointLabels.IsEmpty())
	{
		Kinect::GetLabels(GetArrayBridge(JointLabels));
	}

	TWorldObjectList Objects;

	//	extract skeletons
	auto SkeletonCount = k4abt_frame_get_num_bodies(Frame);
	//std::Debug << "Found " << SkeletonCount << " skeletons" << std::endl;

	//	use k4abt_frame_get_body_index_map to get a label map/mask
	for (auto s = 0; s < SkeletonCount; s++)
	{
		k4abt_skeleton_t Skeleton;
		auto Error = k4abt_frame_get_body_skeleton(Frame, s, &Skeleton);
		Kinect::IsOkay(Error, "k4abt_frame_get_body_skeleton");

		//	gr: for now, if we have multiple skeletons, we're gonna output the default Head,Arm etc 
			//		to match other models, but they are heatmaps and not (yet) outputting proper skeletons
			//		openpose needs to move this way, and output objects
			//		just need a generally nicer way.
			//		In this case, we'll have Head (default) then Head1, Head2 etc as extra skeletons and high 
			//		level can check for the additioanls
		std::string LabelSuffix;
		if (s != 0)
			LabelSuffix += std::to_string(s);

		//	enum the joints
		for (auto j = 0; j < K4ABT_JOINT_COUNT; j++)
		{
			auto& Joint = Skeleton.joints[j];
			TWorldObject Object;
			Object.mScore = Kinect::GetScore(Joint.confidence_level);
			Object.mLabel = JointLabels[j];
			Object.mLabel += LabelSuffix;
			
			const auto MilliToMeters = 0.001f;
			Object.mWorldPosition.x = Joint.position.xyz.x * MilliToMeters;
			Object.mWorldPosition.y = Joint.position.xyz.y * MilliToMeters;
			Object.mWorldPosition.z = Joint.position.xyz.z * MilliToMeters;

			Objects.mObjects.PushBack(Object);
		}
	}

	PushFrame(Objects);
}

CoreMl::TWorldObjectList CoreMl::TKinectAzureSkeletonReader::PopFrame(bool Blocking)
{
	//	if we're blocking, wait for the reader to say there's a frame waiting
	if (Blocking)
	{
		//mOnNewFrameSemaphore.WaitAndReset("TKinectAzureSkeletonReader::PopFrame");
		mOnNewFrameSemaphore.WaitAndReset(nullptr);
	}

	std::lock_guard<std::mutex> Lock(mLastFrameLock);
	auto Copy = mLastFrame;
	mLastFrame.mObjects.Clear(false);
	return Copy;
}

