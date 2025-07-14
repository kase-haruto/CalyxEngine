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
	, public ConfigurableObject<ParticleSystemObjectConfig>{
public:
	// コンストラクタ
	ParticleSystemObject() = default;
	explicit ParticleSystemObject(const std::string& name);
	~ParticleSystemObject() override = default;

	// 更新
	void Initialize();
	void Update() override;
	void ShowGui() override;
	void SetDrawEnable(bool isDrawEnable) override;

	std::string_view GetTypeName() const override{ return "ParticleSystemObject"; }

	// コンフィグ適用・抽出
	void ApplyConfig() override;
	void ExtractConfig() override;

	// 再帰再生など
	void PlayRecursive();
	void StopRecursive();
	void ResetRecursive();
};
