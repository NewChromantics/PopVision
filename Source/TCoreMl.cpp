#if defined(TARGET_OSX) || defined(TARGET_IOS)
#include "SoyAvf.h"
#endif

#include "TCoreMl.h"
#include "HeapArray.hpp"

#if defined(TARGET_WINDOWS)
#include "TWinSkillSkeleton.h"
#endif

#if defined(TARGET_OSX)
#include "TYolo.h"
#include "THourglass.h"
#include "TCpm.h"
#include "TOpenPose.h"
#include "TSsdMobileNet.h"
#include "TMaskRcnn.h"
#include "TDeepLab.h"
//#include "TResnet50.h"
#include "TPosenet.h"
#include "TAppleVisionFace.h"
#endif

namespace CoreMl
{
	std::shared_ptr<CoreMl::TModel> AllocModel(const std::string& Name);
	Array<std::shared_ptr<TModel>> gInstances;
}


__export CoreMl::TModel* PopCoreml_AllocModel(const std::string& Name)
{
	auto Instance = CoreMl::AllocModel(Name);
	if (!Instance)
		return nullptr;

	CoreMl::gInstances.PushBack(Instance);
	CoreMl::TModel* pModel = Instance.get();
	return pModel;
}

std::shared_ptr<CoreMl::TModel> CoreMl::AllocModel(const std::string& Name)
{
#if defined(TARGET_WINDOWS)
	if (Name == TWinSkillSkeleton::ModelName)		return std::make_shared<TWinSkillSkeleton>();
#endif

#if defined(TARGET_OSX)
	if (Name == TYolo::ModelName)		return std::make_shared<TYolo>();
	if (Name == THourglass::ModelName)	return std::make_shared<THourglass>();
	if (Name == TCpm::ModelName)		return std::make_shared<TCpm>();
	if (Name == TOpenPose::ModelName)		return std::make_shared<TOpenPose>();
	if (Name == TSsdMobileNet::ModelName)		return std::make_shared<TSsdMobileNet>();
	if (Name == TMaskRcnn::ModelName)		return std::make_shared<TMaskRcnn>();
	if (Name == TDeepLab::ModelName)		return std::make_shared<TDeepLab>();
	//if (Name == TResnet50::ModelName)		return std::make_shared<TResnet50>();
	if (Name == TPosenet::ModelName)		return std::make_shared<TPosenet>();
	if (Name == TAppleVisionFace::ModelName)		return std::make_shared<TAppleVisionFace>();
#endif 
	std::stringstream Error;
	Error << "Unknown model name " << Name;
	throw Soy::AssertException(Error);
}



#if defined(TARGET_WINDOWS)
void CoreMl::TModel::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("Not Implemented");
}
#endif

void CoreMl::TModel::GetObjects(CVPixelBufferRef Pixels, std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");
}

#if defined(TARGET_WINDOWS)
void CoreMl::TModel::GetLabelMap(const SoyPixelsImpl& Pixels, std::shared_ptr<SoyPixelsImpl>& MapOutput, std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("Not Implemented");
}
#endif

void CoreMl::TModel::GetLabelMap(CVPixelBufferRef Pixels, std::shared_ptr<SoyPixelsImpl>& MapOutput, std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");

}

#if defined(TARGET_WINDOWS)
void CoreMl::TModel::GetLabelMap(const SoyPixelsImpl& Pixels, std::function<void(vec2x<size_t>, const std::string&, ArrayBridge<float>&&)> EnumLabelMap)
{
	throw Soy::AssertException("Not Implemented");
}
#endif

void CoreMl::TModel::GetLabelMap(CVPixelBufferRef Pixels, std::function<void(vec2x<size_t>, const std::string&, ArrayBridge<float>&&)>& EnumLabelMap)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");
}
