#include "TWinSkillSkeleton.h"
#include "MagicEnum/include/magic_enum.hpp"
#include "SoyPixels.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.media.h>
#include <winrt/windows.system.threading.h>

//#include "CameraHelper_cppwinrt.h"
//#include "WindowsVersionHelper.h"
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/windows.foundation.collections.h>
#include <winrt/windows.media.h>
#include <winrt/windows.system.threading.h>


//https://docs.microsoft.com/en-us/windows/uwp/cpp-and-winrt-apis/consume-apis
#include "CameraHelper_cppwinrt.h"
//#include "WindowsVersionHelper.h"

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::System::Threading;
using namespace winrt::Windows::Media;

#include "winrt\Microsoft.AI.Skills.SkillInterfacePreview.h"
#include "winrt\Microsoft.AI.Skills.Vision.SkeletalDetectorPreview.h"
using namespace Microsoft::AI::Skills::SkillInterfacePreview;
using namespace Microsoft::AI::Skills::Vision::SkeletalDetectorPreview;

//	pixels to bitmap
#include "winrt\Windows.Graphics.Imaging.h"
using namespace winrt::Windows::Graphics::Imaging;
#include "winrt\Windows.Storage.Streams.h"
using namespace winrt::Windows::Storage::Streams;


#pragma comment(lib, "windowsapp")


class CoreMl::TWinSkillSkeletonNative
{
public:
	TWinSkillSkeletonNative();

	ISkillDescriptor		mDescriptor;
	SkeletalDetectorBinding	mBinding;
	SkeletalDetectorSkill	mSkill;
};


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
	auto Version = Platform::GetOsVersion();
	if (Version < Soy::TVersion(10, 0, 18362))
		throw "Require windows version >= 10.0.18362";
}



CoreMl::TWinSkillSkeleton::TWinSkillSkeleton() :
	TWinSkill()
{
	mNative.reset( new TWinSkillSkeletonNative() );
}

CoreMl::TWinSkillSkeletonNative::TWinSkillSkeletonNative() :
	mSkill	( nullptr ),
	mBinding	(nullptr)
{
	// Create the SkeletalDetector skill descriptor
	auto SkeltalDescriptor = SkeletalDetectorDescriptor();
	mDescriptor = SkeltalDescriptor.as<ISkillDescriptor>();
			
	//	Create instance of the skill
	mSkill = mDescriptor.CreateSkillAsync().get().as<SkeletalDetectorSkill>();
	//Soy::Debug << "Running Skill on : " << SkillExecutionDeviceKindLookup.at(skill.Device().ExecutionDeviceKind());
	//std::wcout << L" : " << mSkill.Device().Name().c_str() << std::endl;
	
	// Create instance of the skill binding
	mBinding = mSkill.CreateSkillBindingAsync().get().as<SkeletalDetectorBinding>();
}


void CoreMl::TWinSkillSkeleton::GetLabels(ArrayBridge<std::string>&& Labels)
{
	constexpr auto EnumNames = magic_enum::enum_names<JointLabel>();
/*
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
	*/
}


BitmapPixelFormat GetFormat(SoyPixelsFormat::Type Format)
{
	switch (Format)
	{
	case SoyPixelsFormat::RGBA:			return BitmapPixelFormat::Rgba8;
	case SoyPixelsFormat::BGRA:			return BitmapPixelFormat::Bgra8;
	case SoyPixelsFormat::Greyscale:	return BitmapPixelFormat::Gray8;

		//Rgba16 = 12,
		//Gray16 = 57,
		//	Nv12 = 103,
		//	P010 = 104,
			//Yuy2 = 107,

	default:break;
	}

	std::stringstream Error;
	Error << __PRETTY_FUNCTION__ << " unhandled case " << Format;
	throw Soy::AssertException(Error);
}

class RawBuffer : public IBuffer
{

};
/*
class NativeBuffer :
	public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::RuntimeClassType::WinRtClassicComMix>,
	ABI::Windows::Storage::Streams::IBuffer,
	Windows::Storage::Streams::IBufferByteAccess>
{
public:
	virtual ~NativeBuffer()
	{
	}

	STDMETHODIMP RuntimeClassInitialize(byte *buffer, UINT totalSize)
	{
		m_length = totalSize;
		m_buffer = buffer;

		return S_OK;
	}

	STDMETHODIMP Buffer(byte **value)
	{
		*value = m_buffer;

		return S_OK;
	}

	STDMETHODIMP get_Capacity(UINT32 *value)
	{
		*value = m_length;

		return S_OK;
	}

	STDMETHODIMP get_Length(UINT32 *value)
	{
		*value = m_length;

		return S_OK;
	}

	STDMETHODIMP put_Length(UINT32 value)
	{
		m_length = value;

		return S_OK;
	}

private:
	UINT32 m_length;
	byte *m_buffer;
};


Streams::IBuffer ^CreateNativeBuffer(LPVOID lpBuffer, DWORD nNumberOfBytes)
{
	Microsoft::WRL::ComPtr<NativeBuffer> nativeBuffer;
	Microsoft::WRL::Details::MakeAndInitialize<NativeBuffer>(&nativeBuffer, (byte *)lpBuffer, nNumberOfBytes);
	auto iinspectable = (IInspectable *)reinterpret_cast<IInspectable *>(nativeBuffer.Get());
	Streams::IBuffer ^buffer = reinterpret_cast<Streams::IBuffer ^>(iinspectable);

	return buffer;
}*/

void CoreMl::TWinSkillSkeleton::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject)
{
	Soy::TScopeTimerPrint Timer(__PRETTY_FUNCTION__, 0);
	auto& mBinding = mNative->mBinding;
	auto& mSkill = mNative->mSkill;

	BitmapPixelFormat Format = GetFormat(Pixels.GetFormat());
	auto& PixelsArray = Pixels.GetPixelsArray();
	auto* PixelsPointer = PixelsArray.GetArray();
	array_view<const uint8_t> PixelArrayView(PixelsPointer, PixelsPointer + PixelsArray.GetDataSize());

	DataWriter Writer;
	Writer.WriteBytes(PixelArrayView);
	auto PixelBuffer = Writer.DetachBuffer();

	//SoftwareBitmap Bitmap(Format, Pixels.GetWidth(), Pixels.GetHeight());
	auto Bitmap = SoftwareBitmap::CreateCopyFromBuffer(PixelBuffer, Format, Pixels.GetWidth(), Pixels.GetHeight());
	auto Frame = VideoFrame::CreateWithSoftwareBitmap(Bitmap);

	// Set the video frame on the skill binding.
	mBinding.SetInputImageAsync(Frame).get();
	
	// Detect bodies in video frame using the skill
	mSkill.EvaluateAsync(mBinding).get();
	
	auto detectedBodies = mBinding.Bodies();
		
	auto bodyCount = mBinding.Bodies().Size();
	
	auto EnumJoint = [&](auto BodyIndex, Joint Joint,float Score)
	{
		TObject Object;
		//	todo: include body index in label
		Object.mScore = Score;
		Object.mLabel = magic_enum::enum_name(Joint.Label);
		//	uv to pixel
		Object.mGridPos.x = Joint.X * Pixels.GetWidth();
		Object.mGridPos.y = Joint.Y * Pixels.GetHeight();
		auto Texelx = 0.5f / static_cast<float>(Pixels.GetWidth());
		auto Texely = 0.5f / static_cast<float>(Pixels.GetWidth());
		Object.mRect.x = Joint.X - Texelx;
		Object.mRect.y = Joint.Y - Texely;
		Object.mRect.w = 2.0f * Texelx;
		Object.mRect.h = 2.0f * Texely;
		EnumObject(Object);
	};

	for ( auto b=0;	b<detectedBodies.Size();	b++)
	{
		auto Body = detectedBodies.GetAt(b);
		auto Limbs = Body.Limbs();
		for (auto&& Limb : Limbs)
		{
			float Score = 1.0f;
			//	enum each joint
			EnumJoint(b, Limb.Joint1, Score);
			EnumJoint(b, Limb.Joint2, Score);
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
