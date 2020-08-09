#pragma once

#include "PopVision.hpp"


namespace PopVision
{
	class TWinSkillSkeletonNative;
	class TWinSkill;
}

class PopVision::TWinSkill : public PopVision::TModel
{
public:
	TWinSkill();
};

class PopVision::TWinSkillSkeleton : public PopVision::TWinSkill
{
public:
	constexpr static auto ModelName = "WinSkillSkeleton";
public:
	TWinSkillSkeleton();
	
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	virtual void	GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject) override;

	std::shared_ptr<TWinSkillSkeletonNative>	mNative;
};

