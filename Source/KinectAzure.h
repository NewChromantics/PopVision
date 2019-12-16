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
	TKinectAzure();
	
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;

	virtual void	GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject) override;

	std::shared_ptr<TKinectAzureNative>	mNative;
};

