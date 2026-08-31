#pragma once
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * EffectEmitterNodeConfig
	 * - Scene上のEffectに含まれる一つのEmitter Node設定を保持するデータ構造
	 * - SceneObject情報、Emitter設定、描画状態、GPU実行状態を管理する
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief EffectEmitterNodeConfigに関するデータを保持する構造体です。
	 */
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

	/*-----------------------------------------------------------------------------------------
	 * EffectObjectConfig
	 * - Sceneへ配置するEffect全体の保存設定を保持するデータ構造
	 * - 親SceneObject情報と子Emitter Nodeの配列を管理する
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief EffectObjectConfigに関するデータを保持する構造体です。
	 */
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

	/*-----------------------------------------------------------------------------------------
	 * EffectEmitterAssetData
	 * - 再利用可能なEffect Asset内の一Emitter定義を保持するデータ構造
	 * - ローカルTransform、Emitter設定、描画方式を管理する
	 * - Scene固有GUIDや親子関係は保持しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief EffectEmitterAssetDataに関するデータを保持する構造体です。
	 */
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

	/*-----------------------------------------------------------------------------------------
	 * EffectAssetData
	 * - Effect Assetファイル全体のバージョン付き保存データ構造
	 * - Asset名と複数のEmitter定義を管理する
	 * - Runtime EmitterおよびGPUリソースは所有しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief EffectAssetDataに関するデータを保持する構造体です。
	 */
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
