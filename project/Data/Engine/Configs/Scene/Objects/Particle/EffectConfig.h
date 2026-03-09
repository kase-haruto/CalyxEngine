#pragma once
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>

namespace CalyxEffect {
	/* -------------------------------------------------------------
	エフェクト内の1エミッタノード
	------------------------------------------------------------- */
	struct EffectEmitterNodeConfig : public SceneObjectConfig {
		EmitterConfig emitter{};
		bool		  isDrawEnable = true;
		bool		  isGpu		   = false;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EffectEmitterNodeConfig,
									   guid,
									   parentGuid,
									   objectType,
									   name,
									   transform,
									   emitter,
									   isDrawEnable,
									   isGpu)

	/* -------------------------------------------------------------
	   エフェクト全体（親）: SceneObjectConfig を継承
	------------------------------------------------------------- */
	struct EffectObjectConfig : public SceneObjectConfig {
		std::vector<EffectEmitterNodeConfig> emitters;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(EffectObjectConfig,
									   guid,
									   parentGuid,
									   objectType,
									   name,
									   transform,
									   emitters)

} // namespace CalyxEffect