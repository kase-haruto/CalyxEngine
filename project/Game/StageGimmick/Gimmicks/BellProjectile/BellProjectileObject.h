#pragma once
#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"
#include "Engine\Foundation\Utility\Ease\CxEase.h"


class BellProjectileTarget;

/// <summary>
/// 撞木のオブジェクト
/// </summary>
class BellProjectileObject : public StageGimmickObjectBase {
public:
	BellProjectileObject() = default;
	BellProjectileObject(const std::string&			modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~BellProjectileObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "BellProjectileObject";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

	void SetParam(const BellProjectileParam& param) {
		param_ = param;
	}
	void SetIsFlying(bool flag) {
		isFlying_ = flag;
	}
	void SetTarget(BellProjectileTarget* target) {
		target_ = target;
	}

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:
	void ChangeScale() {
		worldTransform_.scale = CalyxEngine::Vector3::Lerp(
			worldTransform_.scale, param_.scale, 0.1f);
	}

private:
	// 鐘のポインタ
	BellProjectileTarget* target_ = nullptr;
	// パラメータ
	BellProjectileParam param_;

	// 飛んでいく座標
	CalyxEngine::Vector3 targetPos_;
	// Parry用の移動状態
	float parryT_		  = 0.0f;
	CalyxEngine::Vector3 parryStartPos_{};

	// 飛んでいるか
	bool isFlying_ = false;
	bool isParry_  = false;
	bool isRing_ = false;
};
