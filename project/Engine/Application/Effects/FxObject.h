#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>

// config
#include <Data/Engine/Configs/Scene/Objects/Particle/EffectConfig.h>

namespace CalyxEffect {

	/*-----------------------------------------------------------------------------------------
	 * FxObject
	 * - エフェクトをシーン上にオブジェクトとして配置するクラス
	 * - 複数のエミッターノードを管理し、再生・停止・設定の保存/読込を提供
	 *---------------------------------------------------------------------------------------*/
	class FxObject final
		: public SceneObject,
		  public IConfigurable {
	public:
		//===================================================================*/
		//					public methods
		//===================================================================*/
		FxObject(const std::string& name = "Fx");
		~FxObject() override;

		//--------- 初期化/更新 ------------------------------------------------
		void Initialize() override;
		void Update(float dt) override;
		void AlwaysUpdate(float dt) override;
		void Destroy() override;

		//--------- Player ----------------------------------------------------
		///< summary>すべてのエミッターを再生</summary>
		void PlayAll() const;
		///< summary>すべてのエミッターを停止</summary>
		void StopAll() const;
		///< summary>すべてのエミッターをリスタート</summary>
		void RestartAll() const;

		void SetAlphaMultiplier(float a);
		void SetCameraFade(float nearZ, float farZ);

		//--------- debugUi ---------------------------------------------------
		void ShowGui() override;
		void LoadFromPath(const std::string& path);

		//--------- json ------------------------------------------------------
		/// <summary>
		/// コンフィグ適用
		/// </summary>
		void ApplyConfig();
		/// <summary>
		///	コンフィグ掃き出し
		/// </summary>
		void ExtractConfig();
		/// <summary>
		///	json空コンフィグの適用
		/// </summary>
		/// ///<param name="j"></param>
		void ApplyConfigFromJson(const nlohmann::json& j) override;
		///< summary>
		/// jsonに掃き出し
		///</summary>
		///< param name="j"></param>
		void ExtractConfigToJson(nlohmann::json& j) const override;

		//--------- accessor --------------------------------------------------
		std::string_view GetTypeName() const override;
		void			 SetWorldPosition(const CalyxMath::Vector3& pos);

	private:
		//===================================================================*/
		//					private methods
		//===================================================================*/
		void RebuildChildrenFromConfig(); // Config 子ノード再構築
		void SyncConfigFromChildren();	  // 子ノード Config 反映

		//--------- add remove ------------------------------------------------
		void RemoveEmitterByGuid(const Guid& id);

		std::shared_ptr<ParticleSystemObject> AddEmitterNode(const EffectEmitterNodeConfig& node);

	private:
		//===================================================================*/
		//					private methods
		//===================================================================*/
		ConfigurableObject<EffectObjectConfig>			 config_;
		std::vector<std::weak_ptr<ParticleSystemObject>> emitters_;
	};

} // namespace CalyxEffect
