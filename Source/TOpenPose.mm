#include "TOpenPose.h"
#include "SoyAvf.h"


//	openpose model from here
//	https://github.com/infocom-tpo/SwiftOpenPose
#import "MobileOpenPose.h"


class CoreMl::TOpenPoseNative
{
public:
	MobileOpenPose*	mOpenPose = [[MobileOpenPose alloc] init];
};

CoreMl::TOpenPose::TOpenPose()
{
	mNative.reset( new TOpenPoseNative );
}

void CoreMl::TOpenPose::GetLabels(ArrayBridge<std::string>&& Labels)
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



void CoreMl::TOpenPose::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto* mOpenPose = mNative->mOpenPose;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mOpenPose predictionFromImage:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
	throw Soy::AssertException( Error );
	
	//	https://github.com/infocom-tpo/SwiftOpenPose/blob/9745c0074dfe7d98265a325e25d2e2bb3d91d3d1/SwiftOpenPose/Sources/Estimator.swift#L122
	//	heatmap rowsxcols is width/8 and height/8 which is 48, which is dim[1] and dim[2]
	//	but 19 features? (dim[0] = 3*19)
	//	https://github.com/tucan9389/PoseEstimation-CoreML/blob/master/PoseEstimation-CoreML/JointViewController.swift#L230
	//	https://github.com/eugenebokhan/iOS-OpenPose/blob/master/iOSOpenPose/iOSOpenPose/CoreML/CocoPairs.swift#L12
	BufferArray<std::string,20> KeypointLabels;
	GetLabels( GetArrayBridge(KeypointLabels) );

	auto BackgroundLabelIndex = 18;
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
	
	auto* ModelOutput = Output.net_output;
	//RunPoseModel( Output.net_output, Pixels, GetKeypointName, EnumObject );
	
	if ( !ModelOutput )
		throw Soy::AssertException("No output from model");
	
	using NUMBER = double;
	BufferArray<int,10> Dim;
	Array<NUMBER> Values;
	ExtractFloatsFromMultiArray( ModelOutput, GetArrayBridge(Dim), GetArrayBridge(Values) );
	
	auto KeypointCount = Dim[0];
	auto HeatmapWidth = Dim[1];
	auto HeatmapHeight = Dim[2];
	auto GetValue = [&](int Keypoint,int HeatmapX,int HeatmapY)
	{
		auto Index = Keypoint * (HeatmapWidth*HeatmapHeight);
		Index += HeatmapX*(HeatmapHeight);
		Index += HeatmapY;
		return Values[Index];
	};
	
	//	same as dim
	//	heatRows = imageWidth / 8
	//	heatColumns = imageHeight / 8
	//	KeypointCount is 57 = 19 + 38
	auto HeatRows = HeatmapWidth;
	auto HeatColumns = HeatmapHeight;
	
	//	https://github.com/infocom-tpo/SwiftOpenPose/blob/9745c0074dfe7d98265a325e25d2e2bb3d91d3d1/SwiftOpenPose/Sources/Estimator.swift#L127
	//	https://github.com/eugenebokhan/iOS-OpenPose/blob/master/iOSOpenPose/iOSOpenPose/CoreML/PoseEstimatior.swift#L72
	//	code above splits into pafMat and HeatmapMat
	auto HeatMatRows = 19;
	auto HeatMatCols = HeatRows*HeatColumns;
	auto heatMat_count = HeatMatRows * HeatMatCols;
	/*
	 NUMBER heatMat[HeatMatRows * HeatMatCols];//[19*HeatRows*HeatColumns];
	 NUMBER pafMat[Values.GetSize() - sizeofarray(heatMat)];//[38*HeatRows*HeatColumns];
	 //let heatMat = Matrix<Double>(rows: 19, columns: heatRows*heatColumns, elements: data )
	 //let separateLen = 19*heatRows*heatColumns
	 //let pafMat = Matrix<Double>(rows: 38, columns: heatRows*heatColumns, elements: Array<Double>(mm[separateLen..<mm.count]))
	 auto heatMat_count = sizeofarray(heatMat);
	 for ( auto i=0;	i<heatMat_count;	i++ )
	 heatMat[i] = Values[i];
	 auto pafMat_count = sizeofarray(pafMat);
	 for ( auto i=0;	i<pafMat_count;	i++ )
	 pafMat[i] = Values[heatMat_count+i];
	 */
	auto* heatMat = Values.GetArray();
	auto* pafMat = &heatMat[heatMat_count];
	
	//	pull coords from heat map
	using vec2i = vec2x<int32_t>;
	Array<vec2i> Coords;
	auto PushCoord = [&](int KeypointIndex,int Index,float Score)
	{
		if ( Score < 0.01f )
		return;
		
		auto y = Index / HeatRows;
		auto x = Index % HeatRows;
		Coords.PushBack( vec2i(x,y) );
		
		auto Label = GetKeypointName(KeypointIndex);
		auto xf = x / static_cast<float>(HeatRows);
		auto yf = y / static_cast<float>(HeatColumns);
		auto wf = 1 / static_cast<float>(HeatRows);
		auto hf = 1 / static_cast<float>(HeatColumns);
		auto Rect = Soy::Rectf( xf, yf, wf, hf );
		
		TObject Object;
		Object.mLabel = Label;
		Object.mScore = Score;
		Object.mRect = Rect;
		Object.mGridPos.x = x;
		Object.mGridPos.y = y;
		EnumObject( Object );
	};
	
	for ( auto r=0;	r<HeatMatRows;	r++ )
	{
		auto KeypointIndex = r;
		static bool SkipBackground = true;
		if ( SkipBackground && KeypointIndex == BackgroundLabelIndex )
		continue;
		auto nms = GetRemoteArray( &heatMat[r*HeatMatCols], HeatMatCols );
		/*
		 std::Debug << "r=" << r << " [ ";
		 for ( int i=0;	i<nms.GetSize();	i++ )
		 {
		 int fi = nms[i] * 100;
		 std::Debug << fi << " ";
		 }
		 std::Debug << " ]" << std::endl;
		 */
		//	get biggest in nms
		//	gr: I think the original code goes through and gets rid of the not-biggest
		//		or only over the threshold?
		//		but the swift code is then doing the check twice if that's the case (and doest need the opencv filter at all)
		/*
		 openCVWrapper.maximum_filter(&data,
		 data_size: Int32(data.count),
		 data_rows: dataRows,
		 mask_size: maskSize,
		 threshold: threshold)
		 */
		
		
		
		static bool BestOnly = false;
		if ( BestOnly )
		{
			auto BiggestCol = 0;
			auto BiggestVal = -1.0f;
			for ( auto c=0;	c<nms.GetSize();	c++ )
			{
				if ( nms[c] <= BiggestVal )
				continue;
				BiggestCol = c;
				BiggestVal = nms[c];
			}
			
			PushCoord( KeypointIndex, BiggestCol, BiggestVal );
		}
		else
		{
			for ( auto c=0;	c<nms.GetSize();	c++ )
			{
				PushCoord( KeypointIndex, c, nms[c] );
			}
		}
		
	}
	/*
	 var coords = [[(Int,Int)]]()
	 for i in 0..<heatMat.rows-1
	 {
	 var nms = Array<Double>(heatMat.row(i))
	 nonMaxSuppression(&nms, dataRows: Int32(heatColumns), maskSize: 5, threshold: _nmsThreshold)
	 let c = nms.enumerated().filter{ $0.1 > _nmsThreshold }.map
	 {
	 x in
	 return ( x.0 / heatRows , x.0 % heatRows )
	 }
	 coords.append(c)
	 }
	 */

}



//	todo: reuse other func but reformat output (then we can make a generic func)
void CoreMl::TOpenPose::GetLabelMap(CVPixelBufferRef Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel)
{
	auto* mOpenPose = mNative->mOpenPose;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mOpenPose predictionFromImage:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
		throw Soy::AssertException( Error );
	
	//	https://github.com/infocom-tpo/SwiftOpenPose/blob/9745c0074dfe7d98265a325e25d2e2bb3d91d3d1/SwiftOpenPose/Sources/Estimator.swift#L122
	//	heatmap rowsxcols is width/8 and height/8 which is 48, which is dim[1] and dim[2]
	//	but 19 features? (dim[0] = 3*19)
	//	https://github.com/tucan9389/PoseEstimation-CoreML/blob/master/PoseEstimation-CoreML/JointViewController.swift#L230
	//	https://github.com/eugenebokhan/iOS-OpenPose/blob/master/iOSOpenPose/iOSOpenPose/CoreML/CocoPairs.swift#L12
	BufferArray<std::string,20> KeypointLabels;
	GetLabels( GetArrayBridge(KeypointLabels) );

	auto BackgroundLabelIndex = 18;
	auto GetKeypointName = [&](size_t Index)
	{
		if ( Index < KeypointLabels.GetSize() )
			return KeypointLabels[Index];
		
		std::stringstream KeypointName;
		KeypointName << "Label_" << Index;
		return KeypointName.str();
	};
	
	auto* ModelOutput = Output.net_output;
	//RunPoseModel( Output.net_output, Pixels, GetKeypointName, EnumObject );
	
	if ( !ModelOutput )
	throw Soy::AssertException("No output from model");
	
	using NUMBER = double;
	BufferArray<int,10> Dim;
	Array<NUMBER> Values;
	ExtractFloatsFromMultiArray( ModelOutput, GetArrayBridge(Dim), GetArrayBridge(Values) );
	
	auto KeypointCount = Dim[0];
	auto HeatmapWidth = Dim[1];
	auto HeatmapHeight = Dim[2];
	auto GetValue = [&](int Keypoint,int HeatmapX,int HeatmapY)
	{
		auto Index = Keypoint * (HeatmapWidth*HeatmapHeight);
		Index += HeatmapX*(HeatmapHeight);
		Index += HeatmapY;
		return Values[Index];
	};
	
	//	same as dim
	//	heatRows = imageWidth / 8
	//	heatColumns = imageHeight / 8
	//	KeypointCount is 57 = 19 + 38
	auto HeatRows = HeatmapWidth;
	auto HeatColumns = HeatmapHeight;
	
	//	https://github.com/infocom-tpo/SwiftOpenPose/blob/9745c0074dfe7d98265a325e25d2e2bb3d91d3d1/SwiftOpenPose/Sources/Estimator.swift#L127
	//	https://github.com/eugenebokhan/iOS-OpenPose/blob/master/iOSOpenPose/iOSOpenPose/CoreML/PoseEstimatior.swift#L72
	//	code above splits into pafMat and HeatmapMat
	auto HeatMatRows = 19;
	auto HeatMatCols = HeatRows*HeatColumns;
	auto heatMat_count = HeatMatRows * HeatMatCols;
	auto* heatMat = Values.GetArray();
	auto* pafMat = &heatMat[heatMat_count];
	
	
	//	generate heat map
	//	gr: make this a float texture!
	Array<uint8_t> MapScores( HeatRows * HeatColumns );
	auto PixelFormat = SoyPixelsFormat::Greyscale;
	MapScores.SetAll(0);
	auto SetMapScore = [&](int x,int y,float Score)
	{
		auto Score8 = static_cast<uint8_t>( Score * 255.f );
		auto Index = x + (y*HeatColumns);
		Score = std::max( MapScores[Index], Score8 );
		MapScores[Index] = Score;
	};
	
	
	//	pull coords from heat map
	using vec2i = vec2x<int32_t>;
	Array<vec2i> Coords;
	auto PushCoord = [&](int KeypointIndex,int Index,float Score)
	{
		auto y = Index / HeatRows;
		auto x = Index % HeatRows;
		SetMapScore( x, y, Score );
	};
	
	for ( auto r=0;	r<HeatMatRows;	r++ )
	{
		auto KeypointIndex = r;
		auto Label = GetKeypointName(KeypointIndex);
		if ( !FilterLabel( Label ) )
			continue;
		
		auto nms = GetRemoteArray( &heatMat[r*HeatMatCols], HeatMatCols );
		
		for ( auto c=0;	c<nms.GetSize();	c++ )
		{
			PushCoord( KeypointIndex, c, nms[c] );
		}
	}
	
	//	write output pixels
	if ( !MapOutput )
		MapOutput.reset( new SoyPixels() );
	
	SoyPixelsRemote MapScoresAsPixels( MapScores.GetArray(), HeatColumns, HeatRows, MapScores.GetDataSize(), PixelFormat );
	MapOutput->Copy( MapScoresAsPixels );
}




//	gr: this is now the cleanest way to get ALL data, and client can filter
void CoreMl::TOpenPose::GetLabelMap(CVPixelBufferRef Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap)
{
	auto* mOpenPose = mNative->mOpenPose;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mOpenPose predictionFromImage:Pixels error:&Error];
	Timer.Stop();
	if ( Error )
		throw Soy::AssertException( Error );
	
	//	https://github.com/infocom-tpo/SwiftOpenPose/blob/9745c0074dfe7d98265a325e25d2e2bb3d91d3d1/SwiftOpenPose/Sources/Estimator.swift#L122
	//	heatmap rowsxcols is width/8 and height/8 which is 48, which is dim[1] and dim[2]
	//	but 19 features? (dim[0] = 3*19)
	//	https://github.com/tucan9389/PoseEstimation-CoreML/blob/master/PoseEstimation-CoreML/JointViewController.swift#L230
	//	https://github.com/eugenebokhan/iOS-OpenPose/blob/master/iOSOpenPose/iOSOpenPose/CoreML/CocoPairs.swift#L12
	BufferArray<std::string,20> KeypointLabels;
	GetLabels( GetArrayBridge(KeypointLabels) );
	
	auto BackgroundLabelIndex = 18;
	auto GetKeypointName = [&](size_t Index)
	{
		if ( Index < KeypointLabels.GetSize() )
			return KeypointLabels[Index];
		
		std::stringstream KeypointName;
		KeypointName << "Label_" << Index;
		return KeypointName.str();
	};
	
	auto* ModelOutput = Output.net_output;
	//RunPoseModel( Output.net_output, Pixels, GetKeypointName, EnumObject );
	
	if ( !ModelOutput )
		throw Soy::AssertException("No output from model");
	
	using NUMBER = double;
	BufferArray<int,10> Dim;
	Array<NUMBER> Values;
	ExtractFloatsFromMultiArray( ModelOutput, GetArrayBridge(Dim), GetArrayBridge(Values) );
	
	auto KeypointCount = Dim[0];
	auto HeatmapWidth = Dim[1];
	auto HeatmapHeight = Dim[2];
	auto GetValue = [&](int Keypoint,int HeatmapX,int HeatmapY)
	{
		auto Index = Keypoint * (HeatmapWidth*HeatmapHeight);
		Index += HeatmapX*(HeatmapHeight);
		Index += HeatmapY;
		return Values[Index];
	};
	
	//	same as dim
	//	heatRows = imageWidth / 8
	//	heatColumns = imageHeight / 8
	//	KeypointCount is 57 = 19 + 38
	auto HeatRows = HeatmapWidth;
	auto HeatColumns = HeatmapHeight;
	
	//	https://github.com/infocom-tpo/SwiftOpenPose/blob/9745c0074dfe7d98265a325e25d2e2bb3d91d3d1/SwiftOpenPose/Sources/Estimator.swift#L127
	//	https://github.com/eugenebokhan/iOS-OpenPose/blob/master/iOSOpenPose/iOSOpenPose/CoreML/PoseEstimatior.swift#L72
	//	code above splits into pafMat and HeatmapMat
	auto HeatMatRows = 19;
	auto HeatMatCols = HeatRows*HeatColumns;
	auto heatMat_count = HeatMatRows * HeatMatCols;
	auto* heatMat = Values.GetArray();
	auto* pafMat = &heatMat[heatMat_count];
	
	//	pull coords from heat map
	//	note: rotated y/x
	vec2x<size_t> MetaSize( HeatRows, HeatColumns );
	Array<float> MapScores;
	
	for ( auto r=0;	r<HeatMatRows;	r++ )
	{
		auto KeypointIndex = r;
		auto Label = GetKeypointName(KeypointIndex);
		
		//	order is rotated, and the data are doubles, so gotta rotate & convert anyway
		auto MapScoresDouble = GetRemoteArray( &heatMat[r*HeatMatCols], HeatMatCols );
		MapScores.SetSize( MapScoresDouble.GetSize() );
		for ( auto i=0;	i<MapScoresDouble.GetSize();	i++ )
		{
			float Value = MapScoresDouble[i];
			
			//	swap x & y
			auto x = i % HeatColumns;
			auto y = i / HeatColumns;
			auto DestinationIndex = y + (x * HeatColumns);
			
			MapScores[DestinationIndex] = Value;
		}
		
		EnumLabelMap( MetaSize, Label, GetArrayBridge( MapScores ) );
	}
}


