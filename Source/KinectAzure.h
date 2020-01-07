#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TKinectAzureNative;
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

	std::shared_ptr<TKinectAzureNative>	mNative;

private:
	void			RequestFrame(bool MustBeAdded);

private:
	//	gr: does this need to be threadsafe/atomic?
	int				mRequestCount = 0;
};

