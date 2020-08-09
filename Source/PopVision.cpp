#if defined(TARGET_OSX) || defined(TARGET_IOS)
#include "SoyAvf.h"
#endif

#include "PopVision.hpp"
#include "SoyTypes.h"
#include "HeapArray.hpp"
#include "TTestSkeleton.h"

#if defined(TARGET_WINDOWS)
#include "TWinSkillSkeleton.h"
#endif

#if defined(ENABLE_KINECTAZURE)
#include "KinectAzure.h"
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


namespace PopVision
{
	const Soy::TVersion	Version(1, 2, 0);
	std::shared_ptr<PopVision::TModel> AllocModel(const std::string& Name);
	Array<std::shared_ptr<TModel>> gInstances;
}

__export int32_t PopVision_GetVersion()
{
	return PopVision::Version.GetMillion();
}


__export PopVision::TModel* PopVision_AllocModel(const std::string& Name)
{
	auto Instance = PopVision::AllocModel(Name);
	if (!Instance)
		return nullptr;

	PopVision::gInstances.PushBack(Instance);
	PopVision::TModel* pModel = Instance.get();
	return pModel;
}

std::shared_ptr<PopVision::TModel> PopVision::AllocModel(const std::string& Name)
{
	if ( Name == TTestSkeleton::ModelName)			return std::make_shared<TTestSkeleton>();
	
#if defined(TARGET_WINDOWS)
	if (Name == TWinSkillSkeleton::ModelName)		return std::make_shared<TWinSkillSkeleton>();
#endif

#if defined(ENABLE_KINECTAZURE)
	if (Name == TKinectAzure::ModelName)			return std::make_shared<TKinectAzure>();
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
void PopVision::TModel::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("Not Implemented");
}
#endif

void PopVision::TModel::GetObjects(CVPixelBufferRef Pixels, std::function<void(const TObject&)>& EnumObject)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");
}

#if defined(TARGET_WINDOWS)
void PopVision::TModel::GetLabelMap(const SoyPixelsImpl& Pixels, std::shared_ptr<SoyPixelsImpl>& MapOutput, std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("Not Implemented");
}
#endif

void PopVision::TModel::GetLabelMap(CVPixelBufferRef Pixels, std::shared_ptr<SoyPixelsImpl>& MapOutput, std::function<bool(const std::string&)>& FilterLabel)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");

}

#if defined(TARGET_WINDOWS)
void PopVision::TModel::GetLabelMap(const SoyPixelsImpl& Pixels, std::function<void(vec2x<size_t>, const std::string&, ArrayBridge<float>&&)> EnumLabelMap)
{
	throw Soy::AssertException("Not Implemented");
}
#endif

void PopVision::TModel::GetLabelMap(CVPixelBufferRef Pixels, std::function<void(vec2x<size_t>, const std::string&, ArrayBridge<float>&&)>& EnumLabelMap)
{
	throw Soy::AssertException("CVPixelBufferRef version should not be being called on this platform");
}

void PopVision::TModel::GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TWorldObject&)>& EnumObject)
{
	throw Soy::AssertException("Get[world]Objects Not Implemented");
}

void PopVision::TModel::SetKinectSmoothing(float Smoothing)
{
	throw Soy::AssertException("SetKinectSmoothing Not Implemented");
}

void PopVision::TModel::SetKinectGpu(int32_t GpuId)
{
	throw Soy::AssertException("SetKinectGpu Not Implemented");
}

void PopVision::TModel::SetKinectTrackMode(uint32_t Mode)
{
	throw Soy::AssertException("SetKinectTrackMode Not Implemented");
}


