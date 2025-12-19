#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include "Game/3dObject/Actor/Player/DangerSense/PlayerDangerSense.h"

#include <Data/Game/Config/Enemy/EnemySpawnerConfig.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/Battle/Movement/Formation/EnemyFormationController.h>

struct IEnemyDirectory;
class EnemyBulletContainer;

class EnemySpawner
	: public SceneObject,
	  public IConfigurable {
public:
	EnemySpawner(const std::string& name = "EnemySpawner");

	/**
	 * \brief 初期化
	 */
	void Initialize() override;
	/**
	 * \brief 更新
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;
	/**
	 * \brief 編隊解散
	 */
	void DissolveFormation();
	/**
	 * \brief スポーンタイマー更新
	 * \param dt デルタタイム
	 */
	void TickSpawnTimer(float dt);
	/**
	 * \brief 即時全スポーン
	 */
	void AlwaysUpdate(float dt) override;
	/**
	 * \brief 即時全スポーン
	 */
	void SpawnAllImmediate();

	//--------- config ------------------------------------------------
	void ApplyConfig();
	void ExtractConfig();
	void ShowGui() override;
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- runtime params ----------------------------------------
	void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }
	void SetSpawnInterval(float interval) { spawnInterval_ = interval; }

	void SetSpawnArea(const CxMath::Vector3& min, const CxMath::Vector3& max) {
		spawnAreaMin_ = min;
		spawnAreaMax_ = max;
	}

	void SetRotationDir(const CxMath::Vector3& dir) { rotationDir_ = dir; }
	void SetRoute(const SplineData& s);
	void SetPlayerTransform(WorldTransform* playerTransform);
	void SetDirectory(IEnemyDirectory* dir) { directory_ = dir; }
	void SetBulletContainer(EnemyBulletContainer* bulletContainer);

	std::string_view GetTypeName() const override { return "EnemySpawner"; }

private:
	/**
	 * \brief 近接判定更新
	 */
	void UpdateProximity();
	/**
	 * \brief 全デスポーン
	 */
	void DespawnAll();
	/**
	 * \brief 敵スポーン
	 */
	void Spawn();
	/**
	 * \brief 死亡敵の掃除
	 */
	void GarbageCollectDead();
	/**
	 * \brief 経路データをJSONから読み込み
	 * \param path ファイルパス
	 * \return 成功したら true
	 */
	bool LoadRouteFromJson(const std::string& path);
	/**
	 * \brief 編隊内オフセット計算
	 * \param index 編隊内インデックス
	 * \return オフセット位置
	 */
	CxMath::Vector3 CalcFormationOffset(size_t index) const;
	/**
	 * \brief 侵入開始位置計算
	 * \param index 編隊内インデックス
	 * \return 侵入開始位置
	 */
	CxMath::Vector3 CalcEntranceStartPos(size_t index) const;

	/**
	 * \brief ２点間距離計算
	 * \param a 点A
	 * \param b 点B
	 * \param useXZ XZ距離を使うか
	 * \return 距離
	 */
	static float Distance_(const CxMath::Vector3& a, const CxMath::Vector3& b, bool useXZ);

private:
	std::list<std::shared_ptr<Enemy>> spawnedEnemies_;
	size_t							  maxSpawnCount_ = 5;

	WorldTransform*		  playerTransform_ = nullptr; // 非所有
	IEnemyDirectory*	  directory_	   = nullptr;
	EnemyBulletContainer* bulletContainer_ = nullptr;

	CxMath::Vector3	   rotationDir_	  = {0, 1, 0};
	float	   rotationSpeed_ = 1.0f;
	SplineData enemyMoveRoute_;

	// タイマーは「アクティブ時のみ」進む
	float spawnTimer_	 = 0.0f;
	float spawnInterval_ = 1.5f;

	CxMath::Vector3 spawnAreaMin_ = {-10.0f, 0.0f, -30.0f};
	CxMath::Vector3 spawnAreaMax_ = {10.0f, 5.0f, -30.0f};

	// ====== 近接起動パラメータ ======
	bool  isActive_			  = false;	// 近接で true、遠離で false
	bool  useXZDistance_	  = true;	// 水平距離で判定（XZ）
	float activationRadius_	  = 150.0f; // 起動半径（以内で起動）
	float deactivationRadius_ = 200.0f; // 停止半径（以上で停止＆デスポーン）

	// ====== 編隊 ======
	EnemyFormationConfig					  formationConfig_; // 設定
	std::unique_ptr<EnemyFormationController> formation_;
	float									  formationTimer_ = 0.0f;

private:
	ConfigurableObject<EnemySpawnerConfig> config_;
	std::string							   moveRoutePath_ = "Resources/Assets/Spline/EnemyMoveRouteR2L.json";
};