#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TAppleVisionFaceNative;
}

class CoreMl::TAppleVisionFace : public CoreMl::TModel
{
public:
	TAppleVisionFace();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject) override;
		
	std::shared_ptr<TAppleVisionFaceNative>	mNative;
};
