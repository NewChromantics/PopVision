#include "TCoreMl.h"
#include "SoyAvf.h"


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
	throw Soy::AssertException("GetLabelMap not implemented in this model");
}


