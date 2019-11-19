#include "THourglass.h"
#include "SoyAvf.h"

#import "Hourglass.h"


class CoreMl::THourglassNative
{
public:
	hourglass*	mHourglass = [[hourglass alloc] init];
};

CoreMl::THourglass::THourglass()
{
	mNative.reset( new THourglassNative );
}

void CoreMl::THourglass::GetLabels(ArrayBridge<std::string>&& Labels)
{
	const std::string ClassLabels[] =
	{
		"Head", "Neck",
		"RightShoulder", "RightElbow", "RightWrist",
		"LeftShoulder", "LeftElbow", "LeftWrist",
		"RightHip", "RightKnee", "RightAnkle",
		"LeftHip", "LeftKnee", "LeftAnkle",
	};
	Labels.PushBackArray( ClassLabels );
}



void CoreMl::THourglass::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto* mHourglass = mNative->mHourglass;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mHourglass predictionFromImage__0:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
	throw Soy::AssertException( Error );
	
	//	https://github.com/tucan9389/PoseEstimation-CoreML/blob/master/PoseEstimation-CoreML/JointViewController.swift#L230
	BufferArray<std::string,20> KeypointLabels;
	GetLabels( GetArrayBridge(KeypointLabels) );
	auto GetKeypointName = [&](size_t Index) -> const std::string&
	{
		//	pad labels
		if ( Index >= KeypointLabels.GetSize() )
		{
			for ( auto i=KeypointLabels.GetSize();	i<=Index;	i++ )
			{
				std::stringstream KeypointName;
				KeypointName << "Label_" << Index;
				KeypointLabels.PushBack( KeypointName.str() );
			}
		}
		
		return KeypointLabels[Index];
	};
	
	RunPoseModel( Output.hourglass_out_3__0, GetKeypointName, EnumObject );
}



void CoreMl::THourglass::GetLabelMap(CVPixelBufferRef Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap)
{
	auto* mHourglass = mNative->mHourglass;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mHourglass predictionFromImage__0:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
		throw Soy::AssertException( Error );
	
	//	https://github.com/tucan9389/PoseEstimation-CoreML/blob/master/PoseEstimation-CoreML/JointViewController.swift#L230
	BufferArray<std::string,20> KeypointLabels;
	GetLabels( GetArrayBridge(KeypointLabels) );
	auto GetKeypointName = [&](size_t Index) -> const std::string&
	{
		//	pad labels
		if ( Index >= KeypointLabels.GetSize() )
		{
			for ( auto i=KeypointLabels.GetSize();	i<=Index;	i++ )
			{
				std::stringstream KeypointName;
				KeypointName << "Label_" << Index;
				KeypointLabels.PushBack( KeypointName.str() );
			}
		}
		
		return KeypointLabels[Index];
	};
	
	RunPoseModel_GetLabelMap( Output.hourglass_out_3__0, GetKeypointName, EnumLabelMap );
}

