#pragma once

#include "PopVision.hpp"


class PopVision::TTestSkeleton : public PopVision::TModel
{
public:
	constexpr static auto ModelName = "TestSkeleton";
	
public:
	TTestSkeleton();
	
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	
	virtual void	GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject) override;
};
