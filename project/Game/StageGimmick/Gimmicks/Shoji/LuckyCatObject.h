#pragma once
#define NOMINMAX
#include <numbers>

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

class ShojiObject;

/// <summary>
/// 招き猫オブジェクトクラス
/// </summary>
class LuckyCatObject : public StageGimmickObjectBase 
{
public:

	LuckyCatObject() = default;
	LuckyCatObject(const std::string& modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~LuckyCatObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "LuckyCatObject";
	}

	void OnCollisionEnter(Collider* other) override;

	void SetParam(const ShojiParam& param) {
		param_ = param;
	}	
	void SetIsFlying(bool flag) {
		isFlying_ = flag;
	}
	void SetShoji(std::array<std::shared_ptr<ShojiObject>, 2> shojiObjs) {
		shojiObjs_ = shojiObjs;
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

	void ChangeScale() {
		worldTransform_.scale = CalyxEngine::Vector3::Lerp(
			worldTransform_.scale, CalyxEngine::Vector3::One() * param_.luckyCatScale, 0.1f);
	}

private:

	// 障子のポインタ
	std::array<std::shared_ptr<ShojiObject>, 2> shojiObjs_;
	// パラメータ
	ShojiParam param_;

	// 飛んでいく座標
	CalyxEngine::Vector3 targetPos_;
	uint32_t shojiIndex_;
	uint32_t randIndex_;
	// Parry用の曲線移動状態
	bool				 parryCurveInit_ = false;
	float				 parryT_		 = 0.0f;
	CalyxEngine::Vector3 parryP0_{}; // 開始点
	CalyxEngine::Vector3 parryP1_{}; // 制御点
	CalyxEngine::Vector3 parryP2_{}; // 終点（target）
	CalyxEngine::Vector3 parryOffsetP2_{};

	// 飛んでいるか
	bool isFlying_ = false;
	bool isParry_  = false;
	bool isShoji_ = false;

};
