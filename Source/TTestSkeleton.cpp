#include "TTestSkeleton.h"
#include <thread>
#include "SoyTime.h"

PopVision::TTestSkeleton::TTestSkeleton()
{
}

void PopVision::TTestSkeleton::GetLabels(ArrayBridge<std::string>&& Labels)
{
	const std::string ClassLabels[] =
	{
		"Head", "Neck",
		"RightShoulder", "RightElbow", "RightWrist",
		"LeftShoulder", "LeftElbow", "LeftWrist",
		"RightHip", "RightKnee", "RightAnkle",
		"LeftHip", "LeftKnee", "LeftAnkle",
		"RightEye",
		"LeftEye",
		"RightEar",
		"LeftEar",
		"Background"
	};
	Labels.PushBackArray( ClassLabels );
}



void PopVision::TTestSkeleton::GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject)
{
	std::this_thread::sleep_for( std::chrono::milliseconds(1000/60) );
	
	//	output a test skeleton shape (make it move!)
	auto Timef = (SoyTime(true).GetTime() % 1000)/1000.0f;
	auto AngleRad = Soy::DegToRad( Timef * 360.0f );
	
	auto Radius = 0.2f;
	auto Size = 0.01f;
	auto x = cosf(AngleRad) * Radius;
	auto y = sinf(AngleRad) * Radius;
	x += 0.5f;
	y += 0.5f;

	TObject Head;
	Head.mLabel = "Head";
	Head.mRect.x = x;
	Head.mRect.y = y;
	Head.mRect.w = Size;
	Head.mRect.h = Size;
	Head.mScore = Timef;
	
	EnumObject( Head );
}
