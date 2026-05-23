#pragma once
#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 鐘オブジェクト
/// </summary>
class BellProjectileTarget : public StageGimmickObjectBase {
public:
	BellProjectileTarget() = default;
	BellProjectileTarget(const std::string&			modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~BellProjectileTarget() override = default;

	std::string_view GetObjectClassName() const override {
		return "BellProjectileTarget";
	}

	void SetParam(const BellProjectileTargetParam& param) {
		param_ = param;
	}

	void SetIsRing(bool ring);
	bool GetIsRing() const { return isRing_; }
	// 鐘部分の座標を取得
	const CalyxEngine::Vector3 GetTargetPos() const;

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:
	void ChangeScale() {
		CalyxEngine::Vector3 targetScale = isRing_ ? param_.hitScale : param_.scale;

		// 鳴った瞬間に少し伸縮させる (Squash and Stretch)
		if(isRing_ && wobbleT_ < 0.5f) {
			float squash = 1.0f + std::sin(wobbleT_ * 30.0f) * 0.2f * std::exp(-wobbleT_ * 10.0f);
			targetScale.y *= (1.0f / squash);
			targetScale.x *= squash;
			targetScale.z *= squash;
		}

		worldTransform_.scale = CalyxEngine::Vector3::Lerp(
			worldTransform_.scale, targetScale, 0.2f);
	}

private:
	// パラメータ
	BellProjectileTargetParam param_;

	// 鐘がなったか（扉が開くか）
	bool isRing_ = false;

	// 揺れのタイマー
	float wobbleT_ = 0.0f;
};
