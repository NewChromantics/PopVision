#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TPosenetNative;
}

class CoreMl::TPosenet : public CoreMl::TModel
{
public:
	constexpr static auto ModelName = "Posenet";
public:
	TPosenet();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	virtual void	GetLabelMap(CVPixelBufferRef Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap) override;

	std::shared_ptr<TPosenetNative>	mNative;
};
