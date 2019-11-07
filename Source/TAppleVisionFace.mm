#include "TAppleVisionFace.h"
#include "SoyAvf.h"

#import <Vision/Vision.h>


class CoreMl::TAppleVisionFaceNative
{
public:
	~TAppleVisionFaceNative()
	{
#if !defined(ARC_ENABLED)
		[Request release];
		[Handler release];
#endif
	}
	
	//	gr: not sure yet which things I can reuse, request I think has to be created every time
	//	gr: the request will find face bounding boxes if none supplied
	//		so we should split this into finding faces
	//		and finding landmarks from face rects
	VNDetectFaceLandmarksRequest*	Request = [[VNDetectFaceLandmarksRequest alloc] init];
	VNSequenceRequestHandler*		Handler = [[VNSequenceRequestHandler alloc] init];
};

CoreMl::TAppleVisionFace::TAppleVisionFace()
{
}

void CoreMl::TAppleVisionFace::GetLabels(ArrayBridge<std::string>&& Labels)
{
	//	gr: can't find the constellation on the request, nor the observation
	//VNRequestFaceLandmarksConstellation65Points
	//VNRequestFaceLandmarksConstellation76Points

	const std::string ClassLabels[] =
	{
		"Face",
	};
	Labels.PushBackArray(ClassLabels);
	
	char LandmarkLabelXX[] = "FaceLandmark00";
	for ( auto i=0;	i<76;	i++ )
	{
		LandmarkLabelXX[12] = '0' + (i / 10);
		LandmarkLabelXX[13] = '0' + (i % 10);
		Labels.PushBack( LandmarkLabelXX );
	}
}



//	note: this wants rgb images

void CoreMl::TAppleVisionFace::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	Soy::TScopeTimerPrint Timer(__func__,0);

	//	todo: see what we can allocate once
	TAppleVisionFaceNative FaceRequest;
	auto* Handler = FaceRequest.Handler;
	auto* Request = FaceRequest.Request;
	NSArray<VNDetectFaceLandmarksRequest*>* Requests = @[Request];

	auto Orientation = kCGImagePropertyOrientationUp;
	NSError* Error = nullptr;
	{
		Soy::TScopeTimerPrint Timer("Perform Requests",5);
		[Handler performRequests:Requests onCVPixelBuffer:Pixels orientation:Orientation error:&Error];
	}
	NSArray<VNFaceObservation*>* Results = Request.results;
	Timer.Stop();
	
	if ( !Results )
		throw Soy::AssertException("Missing results");

	BufferArray<std::string,100> Labels;
	GetLabels( GetArrayBridge(Labels) );
	auto GetFaceLabel = [&]()
	{
		return Labels[0];
	};
	auto GetLandmarkLabel = [&](int LandmarkIndex)
	{
		return Labels[1+LandmarkIndex];
	};
	
	auto ImageSize = Avf::GetSize( Pixels );
	//	gr: is this right? just don't want to send 0 which might look like an invalid rect
	auto OnePxWidth = 1.0 / ImageSize.x;
	auto OnePxHeight = 1.0 / ImageSize.y;
	

	for ( VNFaceObservation* Observation in Results )
	{
		std::Debug << "Got observation" << std::endl;
		//	features are normalised to bounds
		Array<vec2f> Features;
		Array<float> FeatureFloats;
		Soy::Rectf Bounds;
		
		float Roll = 0;
		float Yaw = 0;
		if ( [Observation respondsToSelector:@selector(roll)] )
			Roll = [Observation.roll floatValue];
		if ( [Observation respondsToSelector:@selector(yaw)] )
			Yaw = [Observation.yaw floatValue];
		
		auto FlipNormalisedY = [](float y,float h)
		{
			//	gr: should be using VNImagePointForFaceLandmarkPoint
			auto Bottom = y + h;
			return 1 - Bottom;
		};
		
		Bounds.x = Observation.boundingBox.origin.x;
		Bounds.y = Observation.boundingBox.origin.y;
		Bounds.w = Observation.boundingBox.size.width;
		Bounds.h = Observation.boundingBox.size.height;
		Bounds.y = FlipNormalisedY(Bounds.y,Bounds.h);
		
		//	output a face object
		{
			TObject Face;
			Face.mRect = Bounds;
			//	Observation.description =
			//	<VNFaceObservation: 0x604000184c60> 54DB36B9-7802-423D-B6F8-D679D215EC22 1 [0.500611 0.51542 0.204451 0.255563] ID=0 landmarks 0.446358"
			Face.mLabel = GetFaceLabel();
			Face.mScore = Observation.confidence;
			EnumObject( Face );
		}
		
		//	gr: could change this to per-region, and change the coreml output to a list of points for a feature
		VNFaceLandmarkRegion2D* Landmarks = Observation.landmarks.allPoints;
		//	output each landmrak as an object
		for( auto l=0;	l<Landmarks.pointCount;	l++ )
		{
			//	this is a point inside the bounding box
			auto Point = Landmarks.normalizedPoints[l];
			Point.x = Soy::Lerp( Bounds.Left(), Bounds.Right(), Point.x );
			//	gr: point is upside down inside box
			Point.y = Soy::Lerp( Bounds.Bottom(), Bounds.Top(), Point.y );
			
			TObject Landmark;
			//	gr: center this?
			Landmark.mRect = Soy::Rectf( Point.x, Point.y, OnePxWidth, OnePxHeight );
			Landmark.mLabel = GetLandmarkLabel( l );
			Landmark.mScore = Observation.confidence;
			EnumObject( Landmark );
		}
	}

}

