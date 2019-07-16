#pragma once

#include "TCoreMl.h"

#if defined(__cplusplus)
typedef void TinyYOLO;
#endif

class Coreml::TYolo : public Coreml::TModel
{
public:
	TYolo();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject) override;
	
	TinyYOLO*		mTinyYolo = nullptr;
};
