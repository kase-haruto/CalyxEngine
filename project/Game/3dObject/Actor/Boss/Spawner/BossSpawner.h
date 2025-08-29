#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Game/3dObject/Actor/Boss/Boss.h>

class BossSpawner :
	public SceneObject,
	public IConfigurable {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	BossSpawner(const std::string& name = "BossSpawner");
	void Update(float dt) override;
	void AlwaysUpdate(float dt) override;
	void Spawn();

	//--------- config ------------------------------------------------
	void ApplyConfig();
	void ExtractConfig();
	void ShowGui() override;
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- accessor ------------------------------------------------
	std::string_view GetTypeName() const override { return "BossSpawner"; }


private:
	ConfigurableObject<SceneObjectConfig> config_;
};
