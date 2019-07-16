#pragma once

class SoyPixels;
class SoyPixelsImpl;
class TPixelBuffer;


namespace CoreMl
{
	class TObject;
	
	class TModel;		//	generic interface
	
	class TTinyYolo;
	class THourglass;
	class TCpm;
	class TOpenPose;
	class SsdMobileNet;
	class TMaskRcnn;
	class TDeepLab;
}


class CoreMl::TObject
{
public:
	float			mScore = 0;
	std::string		mLabel;
	Soy::Rectf		mRect = Soy::Rectf(0,0,0,0);
	vec2x<size_t>	mGridPos;
};


class Coreml::TModel
{
public:
	//	get all the labels this model outputs
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels);
	
	//	find objects in map as rects (with some max scoring... map is probbaly better)
	virtual void	GetObjects(const SoyPixelsImpl& Pixels,std::function<void(const TObject&)>& EnumObject);
	
	//	draw found labels on a map
	//	maybe callback says which component/value should go for label
	virtual void	GetLabelMap(const SoyPixelsImpl& Pixels,std::shared_ptr<SoyPixelsImpl>& MapOutput,std::function<bool(const std::string&)>& FilterLabel);
};




