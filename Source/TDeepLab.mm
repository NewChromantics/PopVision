#include "TDeepLab.h"
#include "SoyAvf.h"

#import "DeepLabV3.h"


class CoreMl::TDeepLabNative
{
public:
	DeepLabV3*	mDeepLabv3 = [[DeepLabV3 alloc] init];
};

CoreMl::TDeepLab::TDeepLab()
{
	auto OsVersion = Platform::GetOsVersion();
	auto MinVersion = Soy::TVersion(10,14,2);
	if ( OsVersion < MinVersion )
	{
		std::stringstream Error;
		Error << "Deeplab needs OSX " << MinVersion << " but version is " << OsVersion;
		throw Soy::AssertException(Error);
	}

	mNative.reset( new TDeepLabNative );
}

void CoreMl::TDeepLab::GetLabels(ArrayBridge<std::string>&& Labels)
{
	const std::string ClassLabels[] =
	{
		"aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat",
		"chair", "cow", "diningtable", "dog", "horse", "motorbike", "person",
		"pottedplant", "sheep", "sofa", "train", "tvmonitor"
	};
	Labels.PushBackArray( ClassLabels );
}



void CoreMl::TDeepLab::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto& mDeepLabv3 = mNative->mDeepLabv3;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mDeepLabv3 predictionFromImage:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
		throw Soy::AssertException( Error );
	if ( !Output )
		throw Soy::AssertException("No output from CoreMl prediction");
	
	/// Detections (y1,x1,y2,x2,classId,score) as 6 element vector of doubles
	BufferArray<int,10> ClassBox_Dim;
	Array<float> ClassBox_Values;
	ExtractFloatsFromMultiArray( Output.semanticPredictions, GetArrayBridge(ClassBox_Dim), GetArrayBridge(ClassBox_Values) );
	
	std::Debug << "ClassBox_Dim: [ ";
	for ( int i=0;	i<ClassBox_Dim.GetSize();	i++ )
	std::Debug << ClassBox_Dim[i] << "x";
	std::Debug << " ]" << std::endl;
	
	
	throw Soy::AssertException("Process RCNN output");
}

