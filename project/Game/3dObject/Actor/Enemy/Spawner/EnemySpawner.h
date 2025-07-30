#pragma once
#include <Engine/Objects/3D/Actor/SceneObject.h>


#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/imgui.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Data/Game/Config/Enemy/EnemySpawnerConfig.h>

class EnemyCollection; // 前方宣言
class SceneContext;

class EnemySpawner
	: public SceneObject{
public:
	EnemySpawner(const std::string& name = "EnemySpawner");
	void Update(float dt)override;
	void AlwaysUpdate(float dt)override;
	void ApplyConfig() ;

	void ExtractConfig() ;

	void ShowGui() override;
	void SetOwner(EnemyCollection* owner) { ownerCollection_ = owner; }
	void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }
	void SetSpawnInterval(float interval) { spawnInterval_ = interval; }
	void SetSpawnArea(const Vector3& min, const Vector3& max) {
		spawnAreaMin_ = min;
		spawnAreaMax_ = max;
	}
	void SetPlayerTransform(WorldTransform* playerTransform);
	void SetRotationDir(const Vector3& dir) { rotationDir_ = dir; }

	std::string_view GetTypeName() const override{ return "EnemySpawner"; }

private:
	void Spawn();

private:
	EnemyCollection* ownerCollection_ = nullptr;
	std::list<std::shared_ptr<Enemy>> spawnedEnemies_;
	size_t maxSpawnCount_ = 5;
	WorldTransform worldTransform_;
	WorldTransform* playerTransform_ = nullptr;

	Vector3 rotationDir_;
	float rotationSpeed_ = 1.0f;
	float spawnTimer_ = 0.0f;
	float spawnInterval_ = 5.0f;

	Vector3 spawnAreaMin_ = { -10.0f, 0.0f, -30.0f };
	Vector3 spawnAreaMax_ = { 10.0f, 5.0f, -30.0f };

private:
	ConfigurableObject<EnemySpawnerConfig> config_;
};


