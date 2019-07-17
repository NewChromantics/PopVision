#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TCpmNative;
}

class CoreMl::TCpm : public CoreMl::TModel
{
public:
	TCpm();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject) override;
		
	std::shared_ptr<TCpmNative>	mNative;
};
