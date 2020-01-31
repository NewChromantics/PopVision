#pragma once

#include "TCoreMl.h"


class CoreMl::TTestSkeleton : public CoreMl::TModel
{
public:
	constexpr static auto ModelName = "TestSkeleton";
	
public:
	TTestSkeleton();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	virtual void	GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject) override;
};
