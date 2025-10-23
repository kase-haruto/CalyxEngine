#pragma once

// Engine
#include <Data/Engine/Configs/Scene/Objects/Particle/ParticleSystemObjectConfig.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

// C++
#include <string>

/// <summary>
/// パーティクルオブジェクト
/// </summary>
class ParticleSystemObject
	: public SceneObject,
	  public FxEmitter,
	  public IConfigurable {
public:
	// コンストラクタ
	ParticleSystemObject() = default;
	explicit ParticleSystemObject(const std::string& name);
	~ParticleSystemObject() override;

	void AlwaysUpdate(float dt) override; //< 常時更新
	void ShowGui() override;			  //< debugUI

	// コンフィグ適用・抽出
	void ApplyConfig();
	void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	/// <summary>
	/// セーブ
	/// </summary>
	/// <param name="path"></param>
	void LoadConfig(const std::string& path);

	/// <summary>
	/// ロード
	/// </summary>
	/// <param name="path"></param>
	void SaveConfig(const std::string& path) const;

	void PlayRecursive();		//< 再生
	void StopRecursive();		//< 停止
	void ResetRecursive();		//< リセット

	// accessor
	void			 SetDrawEnable(bool isDrawEnable) override;
	std::string_view GetTypeName() const override { return "ParticleSystemObject"; }
	Vector3			 GetWorldPosition() const;

private:
	// 設定
	ConfigurableObject<ParticleSystemObjectConfig> config_;
};
