#pragma once

#include "TCoreMl.h"


namespace CoreMl
{
	class TWinSkillSkeletonNative;
	class TWinSkill;
}

class CoreMl::TWinSkill : public CoreMl::TModel
{
public:
	TWinSkill();
};

class CoreMl::TWinSkillSkeleton : public CoreMl::TWinSkill
{
public:
	constexpr static auto ModelName = "WinSkillSkeleton";
public:
	TWinSkillSkeleton();
	
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;
	virtual void	GetObjects(const SoyPixelsImpl& Pixels, std::function<void(const TObject&)>& EnumObject) override;

	std::shared_ptr<TWinSkillSkeletonNative>	mNative;
};

