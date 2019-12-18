#include "TCoreMl.h"

void CoreMl::TModel::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("Not Implemented");
}

void CoreMl::TModel::GetObjects(CVPixelBufferRef Pixels, std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");
}

void CoreMl::TModel::GetLabelMap(const SoyPixelsImpl& Pixels, std::shared_ptr<SoyPixelsImpl>& MapOutput, std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("Not Implemented");
}

void CoreMl::TModel::GetLabelMap(CVPixelBufferRef Pixels, std::shared_ptr<SoyPixelsImpl>& MapOutput, std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");

}

void CoreMl::TModel::GetLabelMap(const SoyPixelsImpl& Pixels, std::function<void(vec2x<size_t>, const std::string&, ArrayBridge<float>&&)> EnumLabelMap)
{
	throw Soy::AssertException("Not Implemented");
}

void CoreMl::TModel::GetLabelMap(CVPixelBufferRef Pixels, std::function<void(vec2x<size_t>, const std::string&, ArrayBridge<float>&&)>& EnumLabelMap)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");
}
