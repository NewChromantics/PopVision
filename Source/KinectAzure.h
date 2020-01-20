#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TKinectAzureSkeletonReader;
	class TKinectAzure;
}

class CoreMl::TKinectAzure : public CoreMl::TModel
{
public:
	constexpr static auto ModelName = "KinectAzureSkeleton";
public:
	TKinectAzure();
	
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;

	virtual void	GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TWorldObject&)>& EnumObject) override;
	virtual void	SetKinectSmoothing(float Smoothing) override;

	std::shared_ptr<TKinectAzureSkeletonReader>	mNative;
};

