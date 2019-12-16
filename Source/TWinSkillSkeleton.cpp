#include "TWinSkillSkeleton.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.media.h>
#include <winrt/windows.system.threading.h>

//#include "CameraHelper_cppwinrt.h"
//#include "WindowsVersionHelper.h"
#include <winrt/base.h>
#include <winrt/Microsoft.AI.Skills.SkillInterfacePreview.h>
#include <winrt/Microsoft.AI.Skills.Vision.SkeletalDetectorPreview.h>


using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::System::Threading;
using namespace winrt::Windows::Media;

using namespace Microsoft::AI::Skills::SkillInterfacePreview;
using namespace Microsoft::AI::Skills::Vision::SkeletalDetectorPreview;

// enum to string lookup table for SkillExecutionDeviceKind
static const std::map<SkillExecutionDeviceKind, std::string> SkillExecutionDeviceKindLookup = {
	{ SkillExecutionDeviceKind::Undefined, "Undefined" },
	{ SkillExecutionDeviceKind::Cpu, "Cpu" },
	{ SkillExecutionDeviceKind::Gpu, "Gpu" },
	{ SkillExecutionDeviceKind::Vpu, "Vpu" },
	{ SkillExecutionDeviceKind::Fpga, "Fpga" },
	{ SkillExecutionDeviceKind::Cloud, "Cloud" }
};

// enum to string lookup table for JointLabel
static const std::map<JointLabel, std::string> JointLabelLookup = {
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
};

CoreMl::TWinSkill::TWinSkill()
{
	// Check if we are running Windows 10.0.18362.x or above as required
	HRESULT hr = WindowsVersionHelper::EqualOrAboveWindows10Version(18362);
	IsOkay(hr,"Windows not above win10 18362");
}


class TWinSkillSkeletonNative
{
public:
	TWinSkillSkeletonNative();
	
	ISkillDescriptor		mDescriptor;
	SkeletalDetectorBinding	mBinding;
	SkeletalDetectorSkill	mSkill;
};


CoreMl::TWinSkillSkeleton::TWinSkillSkeleton() :
	TWinSkill()
{
	mNative.reset( new TWinSkillSkeletonNative() );
}

CoreMl::TWinSkillSkeletonNative::TWinSkillSkeletonNative()
{
	// Create the SkeletalDetector skill descriptor
	mDescriptor = SkeletalDetectorDescriptor().as<ISkillDescriptor>();
			
	//	Create instance of the skill
	mSkill = mDescriptor.CreateSkillAsync().get().as<SkeletalDetectorSkill>();
	std::cout << "Running Skill on : " << SkillExecutionDeviceKindLookup.at(skill.Device().ExecutionDeviceKind());
	std::wcout << L" : " << mSkill.Device().Name().c_str() << std::endl;
	
	// Create instance of the skill binding
	mBinding = mSkill.CreateSkillBindingAsync().get().as<SkeletalDetectorBinding>();
}


void Coreml::TWinSkillSkeleton::GetLabels(ArrayBridge<std::string>&& Labels)
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
