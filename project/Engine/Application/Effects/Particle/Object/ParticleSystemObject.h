#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// Engine
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/ParticleSystemObjectConfig.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

// C++
#include <string>

/* ========================================================================
/*		パーティクルをシーンオブジェクトとして使用
/* ===================================================================== */
class ParticleSystemObject
	: public SceneObject
	, public FxEmitter
	, public IConfigurable{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	ParticleSystemObject() = default;
	ParticleSystemObject(const std::string& name);
	~ParticleSystemObject() override;

	void AlwaysUpdate(float dt)override;
	void ShowGui() override;

	//--------- session -----------------------------------------------------
	void PlayRecursive();		//< 再生
	void StopRecursive();		//< 停止
	void ResetRecursive();		//< リセット

	//--------- config -----------------------------------------------------
	// 適用
	void ApplyConfig() ;
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	// 掃き出し
	void ExtractConfig() ;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- save/load -----------------------------------------------------
	// コンフィグのロード
	void LoadConfig(const std::string& path);
	// コンフィグのセーブ
	void SaveConfig(const std::string& path) const;

	//--------- accessor -----------------------------------------------------
	void			 SetDrawEnable(bool isDrawEnable) override;
	std::string_view GetTypeName() const override { return "ParticleSystemObject"; }
	Vector3			 GetWorldPosition() const;

private:
	ConfigurableObject<ParticleSystemObjectConfig> config_;
};
