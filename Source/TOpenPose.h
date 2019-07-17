#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TOpenPoseNative;
}

class CoreMl::TOpenPose : public CoreMl::TModel
{
public:
	TOpenPose();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject) override;

	virtual void	GetLabelMap(CVPixelBufferRef Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel) override;

	std::shared_ptr<TOpenPoseNative>	mNative;
};
