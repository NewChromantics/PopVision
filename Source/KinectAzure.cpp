#include "KinectAzure.h"
#include <k4abt.h>


namespace Kinect
{
	void		IsOkay(k4a_result_t Error, const char* Context);
	void		IsOkay(k4a_wait_result_t Error, const char* Context);
}

class CoreMl::TKinectAzureNative
{
public:
	TKinectAzureNative();
	~TKinectAzureNative();

	k4a_device_t*		mDevice = nullptr;
	k4abt_tracker_t*	mTracker = nullptr;
	k4a_capture_t		mCapture = nullptr;
};



CoreMl::TKinectAzureNative::TKinectAzureNative(size_t DeviceIndex)
{
	auto Error = k4a_device_open(DeviceIndex, &mDevice);
	IsOkay(Error, "k4a_device_open");

	k4a_device_configuration_t Config;
	Error = k4a_device_start_cameras(mDevice, &Config);
	IsOkay(Error, "k4a_device_start_cameras");

	int32_t Timeout = 0;	//	K4A_WAIT_INFINITE
	WaitError = k4a_device_get_capture(mDevice, &mCapture, Timeout);
	IsOkay(WaitError, "k4a_device_get_capture");

	k4a_calibration_t Calibration;
	Error = k4abt_tracker_create(&Calibration, Config, &mTracker);
	IsOkay(Error, "k4abt_tracker_create");

	//	gr: I THINK this creates a queue, not, request-one-frame
	WaitResult = k4abt_tracker_enqueue_capture(mTracker, mCapture, Timeout);
	IsOkay(WaitResult, "k4abt_tracker_enqueue_capture");
}

Coreml::TKinectAzureNative::~TKinectAzureNative()
{
	try
	{
		k4a_capture_release(mCapture);
		k4abt_tracker_shutdown(mTracker);
		k4abt_tracker_destroy(mTracker);
		k4a_device_stop_cameras(mDevice);
		k4a_device_close(mDevice);
	}
	catch (std::exception& e)
	{
		std::Debug << "Error shutting down k4a device " << e.what() << std::endl;
	}
};


Coreml::TKinectAzure::TKinectAzure()
{
	mNative.reset( new TKinectAzureNative() );
}


void CoreMl::TKinectAzure::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject)
{
	//	use k4abt_frame_get_body_index_map to get a label map/mask

	auto& mTracker = mNative->mTracker;

	//	pop skeleton0
	int32_t Timeout = 0;
	k4abt_frame_t Frame;
	auto WaitResult = k4abt_tracker_pop_result(mTracker, &Frame, Timeout);
	Kinect::IsOkay(WaitResult, "k4abt_tracker_pop_result");

	Array<std::string> JointLabels;
	GetLabels(GetArrayBridge(JointLabels));

	auto SkeletonCount = k4abt_frame_get_num_bodies(Frame);
	for ( auto s=0;	s<SkeletonCount;	s++)
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
			Object.mGridPos.x = Joint.position.x * MilliToMeters;
			Object.mGridPos.y = Joint.position.y * MilliToMeters;

			k4a_float3_t     position;    /**< The position of the joint specified in millimeters*/
			k4a_quaternion_t orientation; /**< The orientation of the joint specified in normalized quaternion*/
			k4abt_joint_confidence_level_t confidence_level; /**< The confidence level of the joint*/
		}
			Object.mGridPos = Joint.confidence_level
		}
	)
}


void Coreml::TKinectAzure::GetLabels(ArrayBridge<std::string>&& Labels)
{

	magic_enum::strings<JointLabel>();
	
	{ JointLabel::Nose, "Nose" },
	{ JointLabel::Neck, "Neck" },
	{ JointLabel::RightShoulder, "RightShoulder" },
	{ JointLabel::RightElbow, "RightElbow" },
	{ JointLabel::RightWrist, "RightWrist" },
	{ JointLabel::LeftShoulder, "LeftShoulder" },
	{ JointLabel::LeftElbow, "LeftElbow" },
	{ JointLabel::LeftWrist, "LeftWrist" },
	{ JointLabel::RightHip, "RightHip" },
	{ JointLabel::RightKnee, "RightKnee" },
	{ JointLabel::RightAnkle, "RightAnkle" },
	{ JointLabel::LeftHip, "LeftHip" },
	{ JointLabel::LeftKnee, "LeftKnee" },
	{ JointLabel::LeftAnkle, "LeftAnkle" },
	{ JointLabel::RightEye, "RightEye" },
	{ JointLabel::LeftEye, "LeftEye" },
	{ JointLabel::RightEar, "RightEar" },
	{ JointLabel::LeftEar, "LeftEar" },
	{ JointLabel::NumJoints, "NumJoints" }
}


void Coreml::TWinSkillSkeleton::Run()
{
	auto& Binding = mNative->mBinding;
	auto& Skill = mNative->mSkill;
	
	// Set the video frame on the skill binding.
	Binding.SetInputImageAsync(videoFrame).get();
	
	// Detect bodies in video frame using the skill
	Skill.EvaluateAsync(mBinding).get();
	
	auto detectedBodies = mBinding.Bodies();
		
	auto bodyCount = mBinding.Bodies().Size();
	
	for ( auto i=0;	i<detectedBodies.Size();	i++)
	{
		auto body = detectedBodies.GetAt(i);
		auto limbs = body.Limbs();
		for (auto&& limb : limbs)
		{
			std::cout << JointLabelLookup.at(limb.Joint1.Label) << "-" << JointLabelLookup.at(limb.Joint2.Label) << "|";
		}
	}
	
	
	/*
	// Initialize Camera and register a frame callback handler
	auto cameraHelper = std::shared_ptr<CameraHelper>(
															  CameraHelper::CreateCameraHelper(
																							   [&](std::string failureMessage) // lambda function that acts as callback for failure event
																							   {
																								   std::cerr << failureMessage;
																								   return 1;
																							   },
																							   [&](VideoFrame const& videoFrame) // lambda function that acts as callback for new frame event
																							   {
																								   // Lock context so multiple overlapping events from FrameReader do not race for the resources.
																								   if (!lock.try_lock())
																								   {
																									   return;
																								   }
																								   
																								   // measure time spent binding and evaluating
																								   auto begin = std::chrono::high_resolution_clock::now();
																								   
																								   // Set the video frame on the skill binding.
																								   binding.SetInputImageAsync(videoFrame).get();
																								   
																								   auto end = std::chrono::high_resolution_clock::now();
																								   auto bindTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000.0f;
																								   begin = std::chrono::high_resolution_clock::now();
																								   
																								   // Detect bodies in video frame using the skill
																								   skill.EvaluateAsync(binding).get();
																								   
																								   end = std::chrono::high_resolution_clock::now();
																								   auto evalTime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1000000.0f;
																								   
																								   auto detectedBodies = binding.Bodies();
																								   
																								   // Display bind and eval time
																								   std::cout << "bind: " << bindTime << "ms | ";
																								   std::cout << "eval: " << evalTime << "ms | ";
																								   
																								   int bodyCount = (int)binding.Bodies().Size();
																								   
																								   // Refresh the displayed line with detection result
																								   if (bodyCount > 0)
																								   {
																									   std::cout << "Found "<< bodyCount << " bodies:";
																									   
																									   for (int i = 0; i < (int)detectedBodies.Size(); i++)
																									   {
																										   std::cout << "<-B" << i+1 << "->";
																										   auto body = detectedBodies.GetAt(i);
																										   auto limbs = body.Limbs();
																										   for (auto&& limb : limbs)
																										   {
																											   std::cout << JointLabelLookup.at(limb.Joint1.Label) << "-" << JointLabelLookup.at(limb.Joint2.Label) << "|";
																										   }
																									   }
																								   }
																								   else
																								   {
																									   std::cout << "---------------- No body detected ----------------";
																								   }
																								   std::cout << "\r";
																								   
																								   videoFrame.Close();
																								   
																								   lock.unlock();
																							   }));
			
			std::cout << "\t\t\t\t\t\t\t\t...press enter to Stop" << std::endl;
			
			// Wait for enter keypress
			while (std::cin.get() != '\n');
			
			std::cout << std::endl << "Key pressed.. exiting";
			
			// De-initialize the MediaCapture and FrameReader
			cameraHelper->Cleanup();
		}
		catch (hresult_error const& ex)
		{
			std::wcerr << "Error:" << ex.message().c_str() << ":" << std::hex << ex.code().value << std::endl;
			return ex.code().value;
		}
		return 0;
	}
	catch (hresult_error const& ex)
	{
		std::cerr << "Error:" << std::hex << ex.code() << ":" << ex.message().c_str();
	}
			*/
}
