#include "TMaskRcnn.h"
#include "SoyAvf.h"

//	https://github.com/edouardlp/Mask-RCNN-CoreML
#import "MaskRCNN_MaskRCNN.h"



class CoreMl::TMaskRcnnNative
{
public:
	MaskRCNN_MaskRCNN*	mMaskRcnn = [[MaskRCNN_MaskRCNN alloc] init];
};

CoreMl::TMaskRcnn::TMaskRcnn()
{
	auto OsVersion = Platform::GetOsVersion();
	auto MinVersion = Soy::TVersion(10,13,2);
	if ( OsVersion < MinVersion )
	{
		std::stringstream Error;
		Error << "Deeplab needs OSX " << MinVersion << " but version is " << OsVersion;
		throw Soy::AssertException(Error);
	}

	mNative.reset( new TMaskRcnnNative );
}

void CoreMl::TMaskRcnn::GetLabels(ArrayBridge<std::string>&& Labels)
{
	const std::string ClassLabels[] =
	{
		"aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat",
		"chair", "cow", "diningtable", "dog", "horse", "motorbike", "person",
		"pottedplant", "sheep", "sofa", "train", "tvmonitor"
	};
	Labels.PushBackArray( ClassLabels );
}



void CoreMl::TMaskRcnn::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto* mMaskRcnn = mNative->mMaskRcnn;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__PRETTY_FUNCTION__,0);
	auto Output = [mMaskRcnn predictionFromImage:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
		throw Soy::AssertException( Error );
	if ( !Output )
		throw Soy::AssertException("No output from CoreMl prediction");
	
	/// Detections (y1,x1,y2,x2,classId,score) as 6 element vector of doubles
	BufferArray<int,10> ClassBox_Dim;
	Array<float> ClassBox_Values;
	ExtractFloatsFromMultiArray( Output.detections, GetArrayBridge(ClassBox_Dim), GetArrayBridge(ClassBox_Values) );
	
	std::Debug << "ClassBox_Dim: [ ";
	for ( int i=0;	i<ClassBox_Dim.GetSize();	i++ )
	std::Debug << ClassBox_Dim[i] << "x";
	std::Debug << " ]" << std::endl;
	
	
	throw Soy::AssertException("Process RCNN output");
}

