#pragma once
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>

namespace CalyxEngine {
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

	/* -------------------------------------------------------------
	   Runtime/Asset用のエフェクトデータ
	   SceneObjectとしてのGUIDや親子情報を持たない、再利用可能な定義データ。
	------------------------------------------------------------- */
	struct EffectEmitterAssetData {
		std::string			 name;
		WorldTransformConfig transform;
		EmitterConfig		 emitter{};
		bool				 isDrawEnable = true;
		bool				 isGpu		   = false;
	};

	inline void to_json(nlohmann::json& j, const EffectEmitterAssetData& c) {
		j = nlohmann::json{
			{"name", c.name},
			{"transform", c.transform},
			{"emitter", c.emitter},
			{"isDrawEnable", c.isDrawEnable},
			{"isGpu", c.isGpu}};
	}

	inline void from_json(const nlohmann::json& j, EffectEmitterAssetData& c) {
		c.name		   = j.value("name", std::string{});
		c.transform	   = j.value("transform", WorldTransformConfig{});
		c.isDrawEnable = j.value("isDrawEnable", true);
		c.isGpu		   = j.value("isGpu", false);

		if(j.contains("emitter")) {
			c.emitter = j.at("emitter").get<EmitterConfig>();
		} else {
			// 旧ParticleSystemObjectConfig互換: emitterが直下にあるJSONも受け入れる
			c.emitter.FromJson(j);
		}
	}

	struct EffectAssetData {
		static constexpr uint32_t kCurrentVersion = 1;
		uint32_t version = kCurrentVersion;
		std::string						  name = "Effect";
		std::vector<EffectEmitterAssetData> emitters;
	};

	inline void to_json(nlohmann::json& j, const EffectAssetData& c) {
		j = nlohmann::json{
			{"type", "EffectAsset"},
			{"version", c.version},
			{"name", c.name},
			{"emitters", c.emitters}};
	}

	inline void from_json(const nlohmann::json& j, EffectAssetData& c) {
		// version未定義の既存Assetはversion 0として読み込み、既定値で補完する。
		c.version = j.value("version", 0u);
		c.name = j.value("name", std::string{"Effect"});
		c.emitters.clear();

		if(j.contains("emitters") && j["emitters"].is_array()) {
			for(const auto& emitterJson : j["emitters"]) {
				EffectEmitterAssetData emitter{};
				emitter = emitterJson.get<EffectEmitterAssetData>();
				c.emitters.push_back(std::move(emitter));
			}
		}
		c.version = EffectAssetData::kCurrentVersion;
	}

} // namespace CalyxEngine
