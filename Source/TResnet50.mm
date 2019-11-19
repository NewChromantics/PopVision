#include "TResnet50.h"
#include "SoyAvf.h"

//	from https://github.com/ytakzk/CoreML-samples/blob/master/CoreML-samples/Resnet50.mlmodel
#import "Resnet50.h"



class CoreMl::TResnet50Native
{
public:
	Resnet50*	mResnet50 = [[Resnet50 alloc] init];
};

CoreMl::TResnet50::TResnet50()
{
	mNative.reset( new TResnet50Native );
}

void CoreMl::TResnet50::GetLabels(ArrayBridge<std::string>&& Labels)
{
	const std::string ClassLabels[] =
	{
		"background","person","bicycle","car","motorcycle","airplane","bus",
		"train","truck","boat","traffic light","fire hydrant","???","stop sign","parking meter",
		"bench","bird","cat","dog","horse","sheep","cow","elephant","bear","zebra",
		"giraffe","???","backpack","umbrella","???","???","handbag","tie","suitcase",
		"frisbee","skis","snowboard","sports ball","kite","baseball bat","baseball glove",
		"skateboard","surfboard","tennis racket","bottle","???","wine glass","cup",
		"fork","knife","spoon","bowl","banana","apple","sandwich","orange","broccoli",
		"carrot","hot dog","pizza","donut","cake","chair","couch","potted plant",
		"bed","???","dining table","???","???","toilet","???","tv","laptop","mouse",
		"remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator",
		"???","book","clock","vase","scissors","teddy bear","hair drier","toothbrush"
	};
	Labels.PushBackArray( ClassLabels );
}



void CoreMl::TResnet50::GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject)
{
	auto* mResnet = mNative->mResnet50;
	NSError* Error = nullptr;
	
	Soy::TScopeTimerPrint Timer(__func__,0);
	auto Output = [mResnet predictionFromImage:Pixels error:&Error];
	Timer.Stop();
	if ( !Output )
		throw Soy::AssertException("No output from CoreMl prediction");
	if ( Error )
		throw Soy::AssertException( Error );
	
	Array<std::string> KeypointLabels;
	GetLabels( GetArrayBridge(KeypointLabels) );
	
	auto BackgroundClassIndex = 0;
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
	
	
	//	parse SSD Mobilenet data;
	//	https://github.com/tf-coreml/tf-coreml/issues/107#issuecomment-359675509
	//	Scores for each class (concat_1__0, a 1 x 1 x 91 x 1 x 1917 MLMultiArray)
	//	Anchor-encoded Boxes (concat__0, a 1 x 1 x 4 x 1 x 1917 MLMultiArray)
	using NUMBER = double;
	BufferArray<int,10> ClassScores_Dim;
	Array<NUMBER> ClassScores_Values;
	BufferArray<int,10> AnchorBoxes_Dim;
	Array<NUMBER> AnchorBoxes_Values;
	ExtractFloatsFromMultiArray( Output.concat__0, GetArrayBridge(AnchorBoxes_Dim), GetArrayBridge(AnchorBoxes_Values) );
	ExtractFloatsFromMultiArray( Output.concat_1__0, GetArrayBridge(ClassScores_Dim), GetArrayBridge(ClassScores_Values) );
	/*
	 std::Debug << "Class Scores: [ ";
	 for ( int i=0;	i<ClassScores_Dim.GetSize();	i++ )
	 std::Debug << ClassScores_Dim[i] << "x";
	 std::Debug << " ]" << std::endl;
	 */
	//	Scores for each class (concat_1__0, a 1 x 1 x 91 x 1 x 1917 MLMultiArray)
	//	1917 bounding boxes, 91 classes
	int ClassCount = ClassScores_Dim[2];
	int BoxCount = ClassScores_Dim[4];
	if ( ClassCount != 91 )		throw Soy::AssertException("Expected 91 classes");
	if ( BoxCount != 1917 )	throw Soy::AssertException("Expected 1917 boxes");
	class TClassAndBox
	{
		public:
		float	mScore;
		size_t	mBoxIndex;
		size_t	mClassIndex;
	};
	auto GetBoxClassScore = [&](size_t BoxIndex,size_t ClassIndex)
	{
		auto ValueIndex = (ClassIndex * BoxCount) + BoxIndex;
		auto Score = ClassScores_Values[ValueIndex];
		if ( Score < static_cast<NUMBER>(0.01f) )
		return static_cast<NUMBER>(0.0f);
		return Score;
	};
	Array<TClassAndBox> Matches;
	for ( auto b=0;	b<BoxCount;	b++ )
	{
		//std::Debug << "Box[" << b << "] = [";
		for ( auto c=0;	c<ClassCount;	c++ )
		{
			bool SkipBackground = true;
			if ( SkipBackground && c == BackgroundClassIndex )
			continue;
			
			auto Score = GetBoxClassScore( b, c );
			//std::Debug << " " << int( Score * 100 );
			if ( Score <= 0 )
			continue;
			
			TClassAndBox Match;
			Match.mScore = Score;
			Match.mBoxIndex = b;
			Match.mClassIndex = c;
			Matches.PushBack( Match );
		}
		//std::Debug << " ]" << std::endl;
	}
	
	auto& Anchors = CoreMl::SsdMobileNet_AnchorBounds4;
	auto GetAnchorRect = [&](size_t AnchorIndex)
	{
		//	from here	https://gist.github.com/vincentchu/cf507ed013da0e323d689bd89119c015#file-ssdpostprocessor-swift-L123
		//	I believe the order is y,x,h,w
		auto MinX = Anchors[AnchorIndex][1];
		auto MinY = Anchors[AnchorIndex][0];
		auto MaxX = Anchors[AnchorIndex][3];
		auto MaxY = Anchors[AnchorIndex][2];
		return Soy::Rectf( MinX, MinY, MaxX - MinX, MaxY-MinY );
	};
	
	//	Anchor-encoded Boxes (concat__0, a 1 x 1 x 4 x 1 x 1917 MLMultiArray)
	auto GetAnchorEncodedRect = [&](size_t BoxIndex)
	{
		//	https://github.com/tf-coreml/tf-coreml/issues/107#issuecomment-359675509
		//	The output of the k-th CoreML box is:
		//	ty, tx, th, tw = boxes[0, 0, :, 0, k]
		//	https://github.com/tensorflow/models/blob/master/research/object_detection/box_coders/keypoint_box_coder.py#L128
		//	where x, y, w, h denote the box's center coordinates, width and height
		//	respectively. Similarly, xa, ya, wa, ha denote the anchor's center
		auto Index_y = (0 * BoxCount) + BoxIndex;
		auto Index_x = (1 * BoxCount) + BoxIndex;
		auto Index_h = (2 * BoxCount) + BoxIndex;
		auto Index_w = (3 * BoxCount) + BoxIndex;
		auto x = AnchorBoxes_Values[Index_x];
		auto y = AnchorBoxes_Values[Index_y];
		auto w = AnchorBoxes_Values[Index_w];
		auto h = AnchorBoxes_Values[Index_h];
		
		//	center to top left
		x -= w/2.0f;
		y -= h/2.0f;
		return Soy::Rectf( x, y, w, h );
	};
	
	auto GetAnchorDecodedRect = [&](size_t BoxIndex)
	{
		auto AnchorRect = GetAnchorRect( BoxIndex );
		auto EncodedRect = GetAnchorEncodedRect( BoxIndex );
		
		//	https://github.com/tensorflow/models/blob/master/research/object_detection/box_coders/keypoint_box_coder.py#L128
		//	^ this is the ENCODING of the values, so we need to reverse it
		/*
		 ty = (y - ya) / ha
		 tx = (x - xa) / wa
		 th = log(h / ha)
		 tw = log(w / wa)
		 tky0 = (ky0 - ya) / ha
		 tkx0 = (kx0 - xa) / wa
		 tky1 = (ky1 - ya) / ha
		 tkx1 = (kx1 - xa) / wa
		 */
		//	https://gist.github.com/vincentchu/cf507ed013da0e323d689bd89119c015#file-ssdpostprocessor-swift-L47
		//	You take these and combine them with the anchor boxes using the same routine as this python code.
		//	https://github.com/tensorflow/models/blob/master/research/object_detection/box_coders/keypoint_box_coder.py#L128
		//	Note: You'll need to use the scale_factors of 10.0 and 5.0 here.
		
		//	this is the decoding;
		//	https://gist.github.com/vincentchu/cf507ed013da0e323d689bd89119c015#file-ssdpostprocessor-swift-L47
		auto axcenter = AnchorRect.GetCenterX();
		auto aycenter = AnchorRect.GetCenterY();
		auto ha = AnchorRect.GetHeight();
		auto wa = AnchorRect.GetWidth();
		
		auto ty = EncodedRect.y / 10.0f;
		auto tx = EncodedRect.x / 10.0f;
		auto th = EncodedRect.GetHeight() / 5.0f;
		auto tw = EncodedRect.GetWidth() / 5.0f;
		
		auto w = exp(tw) * wa;
		auto h = exp(th) * ha;
		auto yCtr = ty * ha + aycenter;
		auto xCtr = tx * wa + axcenter;
		auto x = xCtr - (w/2.0f);
		auto y = yCtr - (h/2.0f);
		return Soy::Rectf( x, y, w, h );
	};
	
	//	now extract boxes for matches
	for ( auto i=0;	i<Matches.GetSize();	i++ )
	{
		auto& Match = Matches[i];
		
		auto BoxIndex = Match.mBoxIndex;
		auto Box = GetAnchorDecodedRect( BoxIndex );
		auto ClassName = GetKeypointName( Match.mClassIndex );
		
		TObject Object;
		Object.mLabel = ClassName;
		Object.mScore = Match.mScore;
		Object.mRect = Box;
		EnumObject( Object );
	}
	/*
	 
	 std::Debug << "Anchor Boxes: [ ";
	 for ( int i=0;	i<AnchorBoxes_Dim.GetSize();	i++ )
	 std::Debug << AnchorBoxes_Dim[i] << "x";
	 std::Debug << " ]" << std::endl;
	 */
}


