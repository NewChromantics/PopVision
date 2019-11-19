#include "TPosenet.h"
#include "SoyAvf.h"

#import "posenet513_v1_075.h"

class CoreMl::TPosenetNative
{
public:
	posenet513_v1_075*	mPosenet = [[posenet513_v1_075 alloc] init];
};

CoreMl::TPosenet::TPosenet()
{
	mNative.reset( new TPosenetNative );
}

void CoreMl::TPosenet::GetLabels(ArrayBridge<std::string>&& Labels)
{
	//	list from posenet.js
	//	https://storage.googleapis.com/tfjs-models/demos/posenet/camera.html
	const std::string ClassLabels[17] =
	{
		"Head",	//nose
		"LeftEye", "RightEye", "LeftEar", "RightEar",
		"LeftShoulder",
		"RightShoulder",
		"LeftElbow",
		"RightElbow",
		"LeftWrist",
		"RightWrist",
		"LeftHip",
		"RightHip",
		"LeftKnee",
		"RightKnee",
		"LeftAnkle",
		"RightAnkle",
	};
	Labels.PushBackArray( ClassLabels );
}


	

void CoreMl::TPosenet::GetLabelMap(CVPixelBufferRef Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap)
{
	auto* mPosenet = mNative->mPosenet;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mPosenet predictionFromImage__0:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
		throw Soy::AssertException( Error );
	
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
	
	
	//	gr: looking into this, it extracts good keypoints (scoreIsMaximumInLocalWindow)
	//		filters used ones (withinNmsRadiusOfCorrespondingPoint)
	//		then expands that keypoint to find matching joints for a pose (decodePose)
	//	so for now, lets just get the points!
	//
	/*
	auto scores = Output.heatmap__0;
	auto offset = Output.offset_2_00;
	auto displacementsFwd = Output.displacement_fwd;
	auto displacementsBwd = Output.displacement_bwd;
	auto outputstride = 16;
	auto maxPoseDetections = 15;
	auto ScoreThreadshold = 0.5;
	auto nmsRadius = 20;
	auto squaredNmsRadius = nmsRadius * nmsRadius;
	
	
	BufferArray<int,10> Scores_Dim;
	Array<float> Scores_Values;
	ExtractFloatsFromMultiArray( ModelOutput, GetArrayBridge(Scores_Dim), GetArrayBridge(Scores_Values) );
	
	
	auto RunPoseModel_GetLabelMap = [&](MLMultiArray* ModelOutput,std::function<const std::string&(size_t)> GetKeypointName,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap)
	{
		func decodeMultiplePoses(
								 scores: Tensor, offsets: Tensor,
								 displacementsFwd: Tensor, displacementsBwd: Tensor,
								 outputStride: Int, maxPoseDetections: Int, scoreThreshold: Float = 0.5,
								 nmsRadius : Int = 20) -> [Pose] {
			
			var poses: [Pose] = []
			
			var queue = buildPartWithScoreQueue(scoreThreshold: scoreThreshold,
												localMaximumRadius: kLocalMaximumRadius, scores: scores)
			
			while (poses.count < maxPoseDetections && !queue.isEmpty) {
				let root = queue.dequeue()
				
				// Use of unresolved identifier 'getImageCoords'
				let rootImageCoords =
				getImageCoords(part: root!.part,outputStride: outputStride,offsets: offsets)
				
				if (withinNmsRadiusOfCorrespondingPoint(poses: poses,squaredNmsRadius: squaredNmsRadius,
														vec: rootImageCoords,keypointId: root!.part.id))
				{
					continue
				}
				// Start a new detection instance at the position of the root.
				let keypoints = decodePose(
										   root!, scores, offsets, outputStride, displacementsFwd,
										   displacementsBwd)
				
				let score = getInstanceScore(poses, squaredNmsRadius, keypoints)
				
				poses.append(Pose(keypoints: keypoints, score: score))
			}
			return poses
		}
		
		/*
		if ( !ModelOutput )
			throw Soy::AssertException("No output from model");
		
		BufferArray<int,10> Dim;
		Array<float> Values;
		ExtractFloatsFromMultiArray( ModelOutput, GetArrayBridge(Dim), GetArrayBridge(Values) );
		
		//	parse Hourglass data
		//	https://github.com/tucan9389/PoseEstimation-CoreML/blob/master/PoseEstimation-CoreML/JointViewController.swift#L135
		auto KeypointCount = Dim[0];
		auto HeatmapWidth = Dim[2];
		auto HeatmapHeight = Dim[1];
		auto GetValue = [&](int Keypoint,int HeatmapX,int HeatmapY)
		{
			auto Index = Keypoint * (HeatmapWidth*HeatmapHeight);
			//Index += HeatmapX*(HeatmapHeight);
			//Index += HeatmapY;
			Index += HeatmapY*(HeatmapWidth);
			Index += HeatmapX;
			return Values[Index];
		};
		
		
		//	note: rotated y/x
		vec2x<size_t> MetaSize( HeatmapHeight, HeatmapWidth );
		Array<float> MapScores;
		
		for ( auto k=0;	k<KeypointCount;	k++)
		{
			auto& Label = GetKeypointName(k);
			
			//	order is rotated, and the data are doubles, so gotta rotate & convert anyway
			MapScores.SetSize( HeatmapWidth * HeatmapHeight );
			
			for ( auto x=0;	x<HeatmapWidth;	x++)
			{
				for ( auto y=0;	y<HeatmapHeight;	y++)
				{
					auto Confidence = GetValue( k, x, y );
					auto DestinationIndex = y + (x * HeatmapWidth);
					MapScores[DestinationIndex] = Confidence;
				}
			}
			
			EnumLabelMap( MetaSize, Label, GetArrayBridge( MapScores ) );
		}
	 
	}
	*/
	
	//	heat map is 17(keypoints) x 33(w) x 33(h)
	//	offsets is 34(2*keypoints!) x 33 x 33
	
	RunPoseModel_GetLabelMap( Output.heatmap__0, GetKeypointName, EnumLabelMap );
}


