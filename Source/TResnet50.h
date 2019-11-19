#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TResnet50Native;
}


class CoreMl::TResnet50 : public CoreMl::TModel
{
public:
	TResnet50();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	virtual void	GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject);

	std::shared_ptr<TResnet50Native>	mNative;
};
