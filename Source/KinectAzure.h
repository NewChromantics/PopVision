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
	constexpr static auto	ModelName = "KinectAzureSkeleton";
	const static int32_t	GpuId_Cpu = -1;
	const static int32_t	GpuId_Default = 0;
public:
	TKinectAzure();
	
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;

	virtual void	GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TWorldObject&)>& EnumObject) override;
	virtual void	SetKinectSmoothing(float Smoothing) override;
	virtual void	SetKinectGpu(int32_t GpuId) override;	//	-1 for cpu

	std::shared_ptr<TKinectAzureSkeletonReader>	mNative;
};

