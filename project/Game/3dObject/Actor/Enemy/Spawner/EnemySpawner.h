#pragma once
#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/imgui.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Data/Game/Config/Enemy/EnemySpawnerConfig.h>

struct IEnemyDirectory;

class SceneContext;

class EnemySpawner
	: public SceneObject,
	public IConfigurable {
public:
	EnemySpawner(const std::string& name = "EnemySpawner");
	void Update(float dt) override;
	void AlwaysUpdate(float dt) override;

	//--------- config ------------------------------------------------
	void ApplyConfig();
	void ExtractConfig();
	void ShowGui() override;
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- runtime params ----------------------------------------
	void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }
	void SetSpawnInterval(float interval) { spawnInterval_ = interval; }
	void SetSpawnArea(const Vector3& min, const Vector3& max) { spawnAreaMin_ = min; spawnAreaMax_ = max; }
	void SetRotationDir(const Vector3& dir) { rotationDir_ = dir; }

	// プレイヤーの Transform を注入（親はプレイヤー本人に設定）
	void SetPlayerTransform(WorldTransform* playerTransform);

	// 敵ディレクトリを注入（シーン所有。ポインタは非所有）
	void SetDirectory(IEnemyDirectory* dir) { directory_ = dir; }

	std::string_view GetTypeName() const override { return "EnemySpawner"; }

private:
	void Spawn();
	void GarbageCollectDead();

private:
	std::list<std::shared_ptr<Enemy>> spawnedEnemies_;
	size_t        maxSpawnCount_ = 5;

	WorldTransform  worldTransform_;
	WorldTransform* playerTransform_ = nullptr; // 非所有
	IEnemyDirectory* directory_ = nullptr; // 非所有（GameScene/サービスが所有）

	Vector3 rotationDir_ = { 0,1,0 };
	float   rotationSpeed_ = 1.0f;
	float   spawnTimer_ = 0.0f;
	float   spawnInterval_ = 5.0f;

	Vector3 spawnAreaMin_ = { -10.0f, 0.0f, -30.0f };
	Vector3 spawnAreaMax_ = { 10.0f, 5.0f, -30.0f };

private:
	ConfigurableObject<EnemySpawnerConfig> config_;
};
