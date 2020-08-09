#include "TCpm.h"
#include "SoyAvf.h"

#import "Cpm.h"


namespace PopVision
{
	void		RunPoseModel(MLMultiArray* ModelOutput,const SoyPixelsImpl& Pixels,std::function<std::string(size_t)> GetKeypointName,std::function<void(const PopVision::TObject&)>& EnumObject);
}


class PopVision::TCpmNative
{
public:
	cpm*	mCpm = [[cpm alloc] init];
};

PopVision::TCpm::TCpm()
{
	mNative.reset( new TCpmNative );
}

void PopVision::TCpm::GetLabels(ArrayBridge<std::string>&& Labels)
{
	const std::string ClassLabels[] =
	{
		"Head", "Neck",
		"RightShoulder", "RightElbow", "RightWrist",
		"LeftShoulder", "LeftElbow", "LeftWrist",
		"RightHip", "RightKnee", "RightAnkle",
		"LeftHip", "LeftKnee", "LeftAnkle",
	};
	Labels.PushBackArray(ClassLabels);
}



void PopVision::TCpm::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto* mCpm = mNative->mCpm;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__PRETTY_FUNCTION__,0);
	auto Output = [mCpm predictionFromImage__0:Pixels error:&Error];
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
	
	RunPoseModel( Output.Convolutional_Pose_Machine__stage_5_out__0, GetKeypointName, EnumObject );

}




void PopVision::TCpm::GetLabelMap(CVPixelBufferRef Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap)
{
	auto* mCpm = mNative->mCpm;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__PRETTY_FUNCTION__,0);
	auto Output = [mCpm predictionFromImage__0:Pixels error:&Error];
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
	
	RunPoseModel_GetLabelMap( Output.Convolutional_Pose_Machine__stage_5_out__0, GetKeypointName, EnumLabelMap );
}


