#pragma once

#include "PopVision.hpp"


namespace PopVision
{
	class TDeepLabNative;
}

class PopVision::TDeepLab : public PopVision::TModel
{
public:
	constexpr static auto ModelName = "DeepLab";
public:
	TDeepLab();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject) override;
		
	std::shared_ptr<TDeepLabNative>	mNative;
};
