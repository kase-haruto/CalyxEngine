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
	void PlayAll() const;
	void StopAll() const;
	void RestartAll() const;

	//--------- debugUi ---------------------------------------------------
	void ShowGui() override;
	void LoadFromPath(const std::string& path);

	//--------- json ------------------------------------------------------
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
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