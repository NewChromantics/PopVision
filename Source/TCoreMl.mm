#include "TCoreMl.h"
#include "SoyAvf.h"
#import <CoreML/CoreML.h>

#if __has_feature(objc_arc)
#define ARC_ENABLED
#endif


void CoreMl::TModel::GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto PixelBuffer = Avf::PixelsToPixelBuffer(Pixels);
	
#if !defined(ARC_ENABLED)
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc]init];
#else
	@autoreleasepool
#endif
	{
		auto ReleasePixelBuffer = [&]()
		{
			CVPixelBufferRelease(PixelBuffer);
#if !defined(ARC_ENABLED)
			[pool drain];
#endif
		};
		Soy::TScopeCall ReleasePixels( nullptr, ReleasePixelBuffer );
	
		//	may need to try/catch/ReleasePixelBuffer
		GetObjects( PixelBuffer, EnumObject );
	}
}

	
void CoreMl::TModel::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("GetObjects not implemented in this model");
}


void CoreMl::TModel::GetLabelMap(const SoyPixelsImpl& Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel)
{
	auto PixelBuffer = Avf::PixelsToPixelBuffer(Pixels);
	
#if !defined(ARC_ENABLED)
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc]init];
#else
	@autoreleasepool
#endif
	{
		auto ReleasePixelBuffer = [&]()
		{
			CVPixelBufferRelease(PixelBuffer);
#if !defined(ARC_ENABLED)
			[pool drain];
#endif
		};
		Soy::TScopeCall ReleasePixels( nullptr, ReleasePixelBuffer );
		
		//	may need to try/catch/ReleasePixelBuffer
		GetLabelMap( PixelBuffer, MapOutput, FilterLabel );
	}
}


void CoreMl::TModel::GetLabelMap(CVPixelBufferRef Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("GetLabelMap not implemented in this model");
}


template<typename SOURCETYPE,typename NUMBERTYPE>
void CopyValuesFromVoid(ArrayBridge<NUMBERTYPE>& Dest,void* Source,size_t Count)
{
	auto* SourceValues = reinterpret_cast<SOURCETYPE*>( Source );
	//Dest.SetSize( Count );
	
	auto SourceArray = GetRemoteArray( SourceValues, Count );
	Dest.Copy( SourceArray );
	/*
	 //	for speed
	 auto* DestPtr = Dest.GetArray();
	 for ( auto i=0;	i<Count;	i++ )
	 {
	 auto Sourcef = static_cast<float>( SourceValues[i] );
	 DestPtr[i] = Sourcef;
	 }
	 */
}


void NSArray_NSNumber_ForEach(NSArray<NSNumber*>* Numbers,std::function<void(int64_t)> Enum)
{
	auto Size = [Numbers count];
	for ( auto i=0;	i<Size;	i++ )
	{
		auto* Num = [Numbers objectAtIndex:i];
		auto Integer = [Num integerValue];
		int64_t Integer64 = Integer;
		Enum( Integer64 );
	}
}


template<typename NUMBER>
void ExtractFloatsFromMultiArray(MLMultiArray* MultiArray,ArrayBridge<int>& Dimensions,ArrayBridge<NUMBER>& Values)
{
	Soy::TScopeTimerPrint Timer("ExtractFloatsFromMultiArray", 4);
	
	
	//	get dimensions
	auto PushDim = [&](int64_t DimSize)
	{
		Dimensions.PushBack( static_cast<int>(DimSize) );
	};
	NSArray_NSNumber_ForEach( MultiArray.shape, PushDim );
	
	//	todo: this should match dim
	auto ValueCount = MultiArray.count;
	
	//	convert all values now
	void* MultiArray_DataPointer = MultiArray.dataPointer;
	switch ( MultiArray.dataType )
	{
		case MLMultiArrayDataTypeDouble:
		CopyValuesFromVoid<double>( Values, MultiArray_DataPointer, ValueCount );
		break;
		
		case MLMultiArrayDataTypeFloat32:
		CopyValuesFromVoid<float>( Values, MultiArray_DataPointer, ValueCount );
		break;
		
		case MLMultiArrayDataTypeInt32:
		CopyValuesFromVoid<int32_t>( Values, MultiArray_DataPointer, ValueCount );
		break;
		
		default:
		throw Soy::AssertException("Unhandled grid data type");
	}
	
}

void CoreMl::ExtractFloatsFromMultiArray(MLMultiArray* MultiArray,ArrayBridge<int>&& Dimensions,ArrayBridge<float>&& Values)
{
	::ExtractFloatsFromMultiArray( MultiArray, Dimensions, Values );
}

void CoreMl::ExtractFloatsFromMultiArray(MLMultiArray* MultiArray,ArrayBridge<int>&& Dimensions,ArrayBridge<double>&& Values)
{
	::ExtractFloatsFromMultiArray( MultiArray, Dimensions, Values );
}

void CoreMl::RunPoseModel(MLMultiArray* ModelOutput,std::function<const std::string&(size_t)> GetKeypointName,std::function<void(const TObject&)>& EnumObject)
{
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
	
	
	auto MinConfidence = 0.001f;
	
	for ( auto k=0;	k<KeypointCount;	k++)
	{
		auto& KeypointLabel = GetKeypointName(k);
		
		auto EnumKeypoint = [&](const std::string& Label,float Score,const Soy::Rectf& Rect,size_t GridX,size_t GridY)
		{
			TObject Object;
			Object.mLabel = Label;
			Object.mScore = Score;
			Object.mRect = Rect;
			Object.mGridPos.x = GridX;
			Object.mGridPos.y = GridY;
			
			EnumObject( Object );
		};
		
		for ( auto x=0;	x<HeatmapWidth;	x++)
		{
			for ( auto y=0;	y<HeatmapHeight;	y++)
			{
				auto Confidence = GetValue( k, x, y );
				if ( Confidence < MinConfidence )
					continue;
				
				auto xf = x / static_cast<float>(HeatmapWidth);
				auto yf = y / static_cast<float>(HeatmapHeight);
				auto wf = 1 / static_cast<float>(HeatmapWidth);
				auto hf = 1 / static_cast<float>(HeatmapHeight);
				auto Rect = Soy::Rectf( xf, yf, wf, hf );
				EnumKeypoint( KeypointLabel, Confidence, Rect, x, y );
			}
		}
	
		
	}
	
}



