#pragma once
/* ============================================================================
 *  FormationConfig / EnemyFormationController
 *  - スプラインを使わず、シンプルな数式ベースで隊列の「親」を動かす
 *  - Spawner ごとに 1 個持たせて、Enemy 側から参照する
 *  - ※ Enemy は Camera に親子付けされている前提なので、
 *      ここでの座標は「カメラローカル座標」として扱う
 * ========================================================================== */

#include <functional>
#include <Engine/Foundation/Math/Vector3.h>

/// 隊列の動き種類
enum class EnemyFormationMotionType {
	Straight,  ///< まっすぐ前進
	Snake,     ///< 蛇行しながら前進
	Circle,    ///< 円運動
};

/// スポナー単位で持つ編隊設定
struct EnemyFormationConfig {
	bool useFormation = true;

	EnemyFormationMotionType motionType = EnemyFormationMotionType::Straight;

	// Z 方向の基準位置・進行速度（カメラローカル z 前後）
	float baseZ  = -80.0f;
	float speedZ = 20.0f;

	// Circle 用
	float radius       = 30.0f;
	float angularSpeed = 1.0f;

	// Snake 用
	float snakeAmpX  = 25.0f;
	float snakeAmpY  = 10.0f;
	float snakeFreqX = 2.0f;
	float snakeFreqY = 1.7f;

	// 編隊を維持する時間（秒）
	// 0 以下なら「解散しない」
	float dissolveTime = 0.0f;
};

/// 実際に編隊の「親」の座標を更新するだけの軽いクラス
class EnemyFormationController {
public:
	using MotionFunc = std::function<Vector3(float time)>;

	EnemyFormationController();

	/// 設定からモーション関数を組み立てる
	void Initialize(const EnemyFormationConfig& cfg);

	/// 時間を進めて位置を更新（カメラローカル座標）
	void Update(float dt);

	const Vector3& GetPosition() const { return pos_; }

	/// 経過時間を返す（解散判定などに使う）
	float GetTime() const { return time_; }

	/// 現在の設定を取得
	const EnemyFormationConfig& GetConfig() const { return cfg_; }

private:
	EnemyFormationConfig cfg_;
	MotionFunc      motionFunc_;

	float   time_ = 0.0f;
	Vector3 pos_  = {0, 0, 0}; // Camera ローカル
};
