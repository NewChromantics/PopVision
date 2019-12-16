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
	TWinSkillSkeleton();
	
	virtual void	GetLabels(ArrayBridge<std::string>&& Labels) override;

	std::shared_ptr<TWinSkillSkeletonNative>	mNative;
};

