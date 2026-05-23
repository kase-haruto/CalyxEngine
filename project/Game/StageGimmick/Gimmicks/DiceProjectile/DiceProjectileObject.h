#pragma once
#define NOMINMAX
#include <numbers>

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

class DiceSocketObject;

/// <summary>
/// サイコロのオブジェクト
/// </summary>
class DiceProjectileObject : public StageGimmickObjectBase {
public:
	DiceProjectileObject() = default;
	DiceProjectileObject(const std::string&			modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~DiceProjectileObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "DiceProjectileObject";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

	void SetParam(const DiceProjectileParam& param) {
		param_ = param;
	}
	void SetIsFlying(bool flag) {
		isFlying_ = flag;
	}
	void SetSocket(DiceSocketObject* socket) {
		socket_ = socket;
	}

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:
	CalyxEngine::Vector3 Bezier2(
		const CalyxEngine::Vector3& p0,
		const CalyxEngine::Vector3& p1,
		const CalyxEngine::Vector3& p2,
		float						t) {
		t			  = std::clamp(t, 0.0f, 1.0f);
		const float u = 1.0f - t;
		return (u * u) * p0 + (2.0f * u * t) * p1 + (t * t) * p2;
	}
	void SameNumberRotation();
	void ChangeScale() {
		worldTransform_.scale = CalyxEngine::Vector3::Lerp(
			worldTransform_.scale, CalyxEngine::Vector3::One() * param_.scale, 0.1f);
	}

private:

	// 収納箱のポインタ
	DiceSocketObject* socket_ = nullptr;
	// パラメータ
	DiceProjectileParam param_;

	// 飛んでいく座標
	CalyxEngine::Vector3 targetPos_;
	// Parry用の曲線移動状態
	bool  parryCurveInit_ = false;
	float parryT_		  = 0.0f;
	CalyxEngine::Vector3 parryP0_{}; // 開始点
	CalyxEngine::Vector3 parryP1_{}; // 制御点
	CalyxEngine::Vector3 parryP2_{}; // 終点（target）

	// 飛んでいるか
	bool isFlying_ = false;
	bool isParry_   = false;
	bool isSocket_ = false;
	// 回転時間
	float time_ = 0.0f;

};
