#pragma once
/* ========================================================================
	include space
   ===================================================================== */
// Engine
#include <Data/Engine/Configs/Scene/Objects/Particle/ParticleSystemObjectConfig.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>

// C++
#include <memory>
#include <string>

namespace CalyxEffect {

	/*-----------------------------------------------------------------------------------------
	 * ParticleSystemObject
	 * - パーティクルをシーンオブジェクトとして配置するクラス
	 * - エミッタの管理・設定の保存/読込・シーンとの連携を担当
	 *---------------------------------------------------------------------------------------*/
	class ParticleSystemObject
		: public SceneObject,
		  public IConfigurable {
	public:
		ParticleSystemObject();
		ParticleSystemObject(const std::string& name);
		~ParticleSystemObject() override;

		/* -------- SceneObject overrides -------- */
		void AlwaysUpdate(float dt) override;
		void ShowGui() override;

		/* -------- control -------- */
		void PlayRecursive() const;
		void StopRecursive() const;
		void ResetRecursive() const;

		void Play() const;
		void Stop() const;
		void Reset() const;

		void SetAlphaMultiplier(float a);
		void SetCameraFade(float nearZ, float farZ);

		/* -------- config -------- */
		void ApplyConfig();
		void ApplyConfigFromJson(const nlohmann::json& j) override;
		void ExtractConfig();
		void ExtractConfigToJson(nlohmann::json& j) const override;

		void LoadConfig(const std::string& path);
		void SaveConfig(const std::string& path) const;

		/* -------- accessors -------- */
		void			 SetDrawEnable(bool isDrawEnable) override;
		void			 SetPosition(const CalyxMath::Vector3& pos);
		std::string_view GetTypeName() const override { return "ParticleSystemObject"; }

		const ConfigurableObject<ParticleSystemObjectConfig>& GetConfigObject() const { return config_; }

		std::shared_ptr<CalyxEffect::FxEmitter> GetEmitter() const { return emitter_; }

	private:
		ConfigurableObject<ParticleSystemObjectConfig> config_;

		std::shared_ptr<CalyxEffect::FxEmitter> emitter_;
	};
} // namespace CalyxEffect
