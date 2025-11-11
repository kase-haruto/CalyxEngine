#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>

// config
#include <Data/Engine/Configs/Scene/Objects/Particle/EffectConfig.h>

/* ========================================================================
/*		エフェクトをシーン上にオブジェクトとして配置するクラス
/* ===================================================================== */
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

	//--------- Player ----------------------------------------------------
	///<summary>すべてのエミッターを再生</summary>
	void PlayAll() const;
	///<summary>すべてのエミッターを停止</summary>
	void StopAll() const;
	///<summary>すべてのエミッターをリスタート</summary>
	void RestartAll() const;

	//--------- debugUi ---------------------------------------------------
	void ShowGui() override;

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
	///<summary>
	///jsonに掃き出し
	///</summary>
	///<param name="j"></param>
	void ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- accessor --------------------------------------------------
	std::string_view GetTypeName() const override;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	void RebuildChildrenFromConfig(); // Config 子ノード再構築
	void SyncConfigFromChildren();    // 子ノード Config 反映

	//--------- add remove ------------------------------------------------
	void RemoveEmitterByGuid(const Guid& id);

	std::shared_ptr<ParticleSystemObject> AddEmitterNode(const EffectEmitterNodeConfig& node);

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	ConfigurableObject<EffectObjectConfig>             config_;
	std::vector<std::shared_ptr<ParticleSystemObject>> emitters_;
};