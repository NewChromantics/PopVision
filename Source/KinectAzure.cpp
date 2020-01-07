#include "KinectAzure.h"
#include <k4abt.h>
#include <k4a/k4a.h>
#include "SoyDebug.h"
#include "HeapArray.hpp"
#include "MagicEnum/include/magic_enum.hpp"

#if !defined(ENABLE_KINECTAZURE)
#error Expected ENABLE_KINECTAZURE to be defined
#endif

namespace Kinect
{
	void		IsOkay(k4a_result_t Error, const char* Context);
	void		IsOkay(k4a_wait_result_t Error, const char* Context);
	void		InitDebugHandler();
}

class CoreMl::TKinectAzureNative
{
public:
	TKinectAzureNative(size_t DeviceIndex);
	~TKinectAzureNative();

private:
	void				Shutdown();

public:
	k4a_device_t		mDevice = nullptr;
	k4abt_tracker_t		mTracker = nullptr;
	k4a_capture_t		mCapture = nullptr;
};


void Kinect::InitDebugHandler()
{
	auto OnDebug = [](void* Context, k4a_log_level_t Level, const char* Filename, const int Line, const char* Message)
	{
		std::Debug << "Kinect[" << magic_enum::enum_name(Level) << "] " << Filename << "(" << Line << "): " << Message << std::endl;
	};

	void* Context = nullptr;
	auto Result = k4a_set_debug_message_handler(OnDebug, Context, K4A_LOG_LEVEL_TRACE);
	IsOkay(Result, "k4a_set_debug_message_handler");

	//	set these env vars for extra logging
	//	https://github.com/microsoft/Azure-Kinect-Sensor-SDK/issues/709
	//Platform::SetEnvVar("K4ABT_ENABLE_LOG_TO_A_FILE", "k4abt.log");
	//Platform::SetEnvVar("K4ABT_LOG_LEVEL", "i");	//	or W
}


CoreMl::TKinectAzureNative::TKinectAzureNative(size_t DeviceIndex)
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

		int32_t Timeout = 1000;	//	K4A_WAIT_INFINITE
		auto WaitError = k4a_device_get_capture(mDevice, &mCapture, Timeout);
		Kinect::IsOkay(WaitError, "k4a_device_get_capture");

		k4a_calibration_t Calibration;
		Error = k4a_device_get_calibration(mDevice, DeviceConfig.depth_mode, DeviceConfig.color_resolution, &Calibration);
		Kinect::IsOkay(WaitError, "k4a_device_get_calibration");

		//	gr: as this takes a while to load, stop the cameras, then restart when needed
		k4a_device_stop_cameras(mDevice);
		
		k4abt_tracker_configuration_t TrackerConfig = K4ABT_TRACKER_CONFIG_DEFAULT;
		Error = k4abt_tracker_create(&Calibration, TrackerConfig, &mTracker);
		Kinect::IsOkay(Error, "k4abt_tracker_create");
	}
	catch (std::exception& e)
	{
		Shutdown();
		throw;
	}
}

CoreMl::TKinectAzureNative::~TKinectAzureNative()
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

void CoreMl::TKinectAzureNative::Shutdown()
{
	k4a_capture_release(mCapture);
	k4abt_tracker_shutdown(mTracker);
	k4abt_tracker_destroy(mTracker);
	k4a_device_stop_cameras(mDevice);
	k4a_device_close(mDevice);
}


CoreMl::TKinectAzure::TKinectAzure()
{
	Kinect::InitDebugHandler();

	auto DeviceCount = k4a_device_get_installed_count();
	std::Debug << "KinectDevice count: " << DeviceCount << std::endl;

	auto DeviceIndex = 0;
	mNative.reset( new TKinectAzureNative(DeviceIndex) );

	//	do an initial request, assuming user is probably going to want one
	RequestFrame(true);
}


void CoreMl::TKinectAzure::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject)
{
	auto& mTracker = mNative->mTracker;

	//	this func should probably be on a "wait for capture" thread, so lets timeout a bit, 
	//	rather than fail if nothing is ready
	//	pop skeletons
	int32_t TimeoutMs = 50;
	k4abt_frame_t Frame;
	auto WaitResult = k4abt_tracker_pop_result(mTracker, &Frame, TimeoutMs);
	//	gr: if this fails, should we figure out if we have any requests queued up?
	{
		std::stringstream Error;
		Error << "k4abt_tracker_pop_result (RequestCount=" << mRequestCount << ")";
		Kinect::IsOkay(WaitResult, Error.str().c_str());
	}
	mRequestCount--;

	//	enum results
	Array<std::string> JointLabels;
	GetLabels(GetArrayBridge(JointLabels));

	auto SkeletonCount = k4abt_frame_get_num_bodies(Frame);
	std::Debug << "Found " << SkeletonCount << " skeletons" << std::endl;
	//	use k4abt_frame_get_body_index_map to get a label map/mask
	for (auto s = 0; s < SkeletonCount; s++)
	{
		k4abt_skeleton_t Skeleton;
		auto Error = k4abt_frame_get_body_skeleton(Frame, s, &Skeleton);
		Kinect::IsOkay(Error, "k4abt_frame_get_body_skeleton");

		//	enum the joints
		for (auto j = 0; j < K4ABT_JOINT_COUNT; j++)
		{
			auto& Joint = Skeleton.joints[j];
			TObject Object;
			Object.mScore = Joint.confidence_level;
			Object.mLabel = JointLabels[j];

			//	todo: support 3d! (and this isn't in screen space!)
			const auto MilliToMeters = 0.001f;
			Object.mGridPos.x = Joint.position.xyz.x * MilliToMeters;
			Object.mGridPos.y = Joint.position.xyz.y * MilliToMeters;
			Object.mScore = Joint.confidence_level;

			EnumObject(Object);
		}
	}

	RequestFrame(true);
}

void CoreMl::TKinectAzure::RequestFrame(bool MustBeAdded)
{
	auto& mTracker = mNative->mTracker;
	auto& mCapture = mNative->mCapture;

	//	queues up a "find the skeleton" request 
	//	gr: to avoid problems, lets not use infinite, but fail if it takes longer than a sec
	//		ie. there have been too many or OOM, we should fail and caller could restart
	auto Timeout = MustBeAdded ? 1000 : 0;
	auto WaitError = k4abt_tracker_enqueue_capture(mTracker, mCapture, Timeout);

	//	gr: don't throw if not MustBeAdded?
	Kinect::IsOkay(WaitError, "k4abt_tracker_enqueue_capture");

	mRequestCount++;
}


//	remap enum to allow constexpr for magic_enum
namespace Kinect
{
	namespace JointLabel
	{
		enum TYPE
		{
			EyeLeft = K4ABT_JOINT_EYE_LEFT,
			EarLeft = K4ABT_JOINT_EAR_LEFT,
			EyeRight = K4ABT_JOINT_EYE_RIGHT,
			EarRight = K4ABT_JOINT_EAR_RIGHT,
			Head = K4ABT_JOINT_HEAD,
			Neck = K4ABT_JOINT_NECK,
			Nose = K4ABT_JOINT_NOSE,
			Chest = K4ABT_JOINT_SPINE_CHEST,
			Navel = K4ABT_JOINT_SPINE_NAVEL,
			Pelvis = K4ABT_JOINT_PELVIS,
			
			ClavicleLeft = K4ABT_JOINT_CLAVICLE_LEFT,
			ShoulderLeft = K4ABT_JOINT_SHOULDER_LEFT,
			ElbowLeft = K4ABT_JOINT_ELBOW_LEFT,
			WristLeft = K4ABT_JOINT_WRIST_LEFT,
			HandLeft = K4ABT_JOINT_HAND_LEFT,
			FingerLeft = K4ABT_JOINT_HANDTIP_LEFT,
			ThumbLeft = K4ABT_JOINT_THUMB_LEFT,
			HipLeft = K4ABT_JOINT_HIP_LEFT,
			KneeLeft = K4ABT_JOINT_KNEE_LEFT,
			AnkleLeft = K4ABT_JOINT_ANKLE_LEFT,
			FootLeft = K4ABT_JOINT_FOOT_LEFT,

			ClavicleRight = K4ABT_JOINT_CLAVICLE_RIGHT,
			ShoulderRight = K4ABT_JOINT_SHOULDER_RIGHT,
			ElbowRight = K4ABT_JOINT_ELBOW_RIGHT,
			WristRight = K4ABT_JOINT_WRIST_RIGHT,
			HandRight = K4ABT_JOINT_HAND_RIGHT,
			FingerRight = K4ABT_JOINT_HANDTIP_RIGHT,
			ThumbRight = K4ABT_JOINT_THUMB_RIGHT,
			HipRight = K4ABT_JOINT_HIP_RIGHT,
			KneeRight = K4ABT_JOINT_KNEE_RIGHT,
			AnkleRight = K4ABT_JOINT_ANKLE_RIGHT,
			FootRight = K4ABT_JOINT_FOOT_RIGHT,
			
			
			//Count = K4ABT_JOINT_COUNT
		};
	}
}


void CoreMl::TKinectAzure::GetLabels(ArrayBridge<std::string>&& Labels)
{
	auto Names = magic_enum::enum_names<Kinect::JointLabel::TYPE>();
	for (auto&& Name : Names)
	{
		Labels.PushBack( std::string(Name) );
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
