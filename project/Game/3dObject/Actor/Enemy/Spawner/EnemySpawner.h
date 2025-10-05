#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Data/Game/Config/Enemy/EnemySpawnerConfig.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>

struct IEnemyDirectory;

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

	void SetPlayerTransform(WorldTransform* playerTransform);
	void SetDirectory(IEnemyDirectory* dir) { directory_ = dir; }

	std::string_view GetTypeName() const override { return "EnemySpawner"; }

private:
	void UpdateProximity_();
	void DespawnAll_();

	void Spawn();
	void GarbageCollectDead();

	// ====== util ======
	static float Distance_(const Vector3& a, const Vector3& b, bool useXZ);

private:
	std::list<std::shared_ptr<Enemy>> spawnedEnemies_;
	size_t maxSpawnCount_ = 5;

	WorldTransform* playerTransform_ = nullptr; // 非所有
	IEnemyDirectory* directory_ = nullptr;

	Vector3 rotationDir_ = { 0,1,0 };
	float   rotationSpeed_ = 1.0f;

	// タイマーは「アクティブ時のみ」進む
	float   spawnTimer_ = 5.0f;
	float   spawnInterval_ = 5.0f;

	Vector3 spawnAreaMin_ = { -10.0f, 0.0f, -30.0f };
	Vector3 spawnAreaMax_ = { 10.0f, 5.0f, -30.0f };

	// ====== 近接起動パラメータ ======
	bool  isActive_ = false;   // 近接で true、遠離で false
	bool  useXZDistance_ = true;    // 水平距離で判定（XZ）
	float activationRadius_ = 200.0f;   // 起動半径（以内で起動）
	float deactivationRadius_ = 100.0f;  // 停止半径（以上で停止＆デスポーン）

private:
	ConfigurableObject<EnemySpawnerConfig> config_;
};
