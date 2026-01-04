#pragma once

// Engine
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/ParticleSystemObjectConfig.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

// C++
#include <string>

class ParticleSystemObject
	: public SceneObject
	, public FxEmitter
	, public IConfigurable{
public:
	// コンストラクタ
	ParticleSystemObject() = default;
	explicit ParticleSystemObject(const std::string& name);
	~ParticleSystemObject() override;

	// 更新
	void Initialize();
	void Update(float dt)override;
	void AlwaysUpdate(float dt)override;
	void ShowGui() override;
	void SetDrawEnable(bool isDrawEnable) override;

	std::string_view GetTypeName() const override{ return "ParticleSystemObject"; }
	Vector3 GetWorldPosition() const {
		return GetWorldTransform().GetWorldPosition();
	}

	// コンフィグ適用・抽出
	void ApplyConfig() ;
	void ExtractConfig() ;
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;
	void LoadConfig(const std::string& path){ config_.LoadConfig(path);  ApplyConfig(); }
	void SaveConfig(const std::string& path) const{
		const_cast< ParticleSystemObject* >(this)->ExtractConfig();
		config_.SaveConfig(path);
	}

	// 再帰再生など
	void PlayRecursive();
	void StopRecursive();
	void ResetRecursive();

private:
	ConfigurableObject<ParticleSystemObjectConfig> config_;
};
