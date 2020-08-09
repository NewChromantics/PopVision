#pragma once

class SoyPixels;
class SoyPixelsImpl;
class TPixelBuffer;

#include "PopVision.hpp"
#include "Array.hpp"
#include "SoyVector.h"


//	forward declaration
#if defined(__OBJC__)
@class MLMultiArray;
#endif

#if defined(TARGET_WINDOWS) || defined(TARGET_ANDROID)
typedef void* CVPixelBufferRef;
#else
#include <CoreVideo/CoreVideo.h>
#endif

namespace PopVision
{
	//	deprecate objects for more context specific;
	//		pixelmaps (per label?)
	//		world-space labels
	//		rect labels
	class TObject;
	class TWorldObject;
	
	class TModel;		//	generic interface

	//	Test Models (should be for every platform)
	class TTestSkeleton;

	//	CoreML models
	class TYolo;
	class THourglass;
	class TCpm;
	class TOpenPose;
	class TSsdMobileNet;
	class TMaskRcnn;
	class TDeepLab;
	class TResnet50;
	class TPosenet;
	
	//	Apple vision framework
	class TAppleVisionFace;
	
	//	WindowsMl Windows Skills
	class TWinSkillSkeleton;

	class TKinectAzure;
	
#if defined(__OBJC__)
	void		RunPoseModel(MLMultiArray* ModelOutput,std::function<const std::string&(size_t)> GetKeypointName,std::function<void(const TObject&)>& EnumObject);
	void		RunPoseModel_GetLabelMap(MLMultiArray* ModelOutput,std::function<const std::string&(size_t)> GetKeypointName,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap);
	void		ExtractFloatsFromMultiArray(MLMultiArray* MultiArray,ArrayBridge<int>&& Dimensions,ArrayBridge<float>&& Values);
	void		ExtractFloatsFromMultiArray(MLMultiArray* MultiArray,ArrayBridge<int>&& Dimensions,ArrayBridge<double>&& Values);
#endif
}


//	need to redo this for different spaces/combinations
//	maybe different funcs for different objects
//	grid is for low-res scores
//	rects are for rects
//	world pos for 3D sensors
class PopVision::TObject
{
public:
	float			mScore = 0;
	std::string		mLabel;

	Soy::Rectf		mRect = Soy::Rectf(0,0,0,0);
	vec2x<size_t>	mGridPos;
};

class PopVision::TWorldObject
{
public:
	float			mScore = 0;
	std::string		mLabel;
	vec3f			mWorldPosition;	//	expected to be in metres
	uint64_t		mTimeMs = 0;
};


class PopVision::TModel
{
public:
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels)=0;
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject);
	virtual void	GetObjects(CVPixelBufferRef Pixels,std::function<void(const TObject&)>& EnumObject);

	//	world space objects/labels (eg. kinect skeleton)
	virtual void	GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TWorldObject&)>& EnumObject);
	
	//	draw found labels on a map
	//	maybe callback says which component/value should go for label
	virtual void	GetLabelMap(const SoyPixelsImpl& Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel);
	virtual void	GetLabelMap(CVPixelBufferRef Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel);

	virtual void	GetLabelMap(const SoyPixelsImpl& Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)> EnumLabelMap);
	virtual void	GetLabelMap(CVPixelBufferRef Pixels,std::function<void(vec2x<size_t>,const std::string&,ArrayBridge<float>&&)>& EnumLabelMap);

	//	options for kinect, anything else would need a per-request option...
	virtual void	SetKinectSmoothing(float Smoothing);
	virtual void	SetKinectGpu(int32_t GpuId);
	virtual void	SetKinectTrackMode(uint32_t Mode);
};

//	C++ factory for dll
//	todo: make this a proper CAPI for unity etc, and some nice dumb interface (instance ID etc)
//	todo: EnumModels()
__export PopVision::TModel*	PopVision_AllocModel(const std::string& Name);



