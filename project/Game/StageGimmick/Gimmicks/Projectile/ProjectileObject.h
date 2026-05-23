#pragma once

#include "Game/StageGimmick/Base/StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 飛んでくる弾のオブジェクトクラス
/// </summary>
class ProjectileObject : public StageGimmickObjectBase 
{
public:

	ProjectileObject() = default;
	ProjectileObject(const std::string& modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~ProjectileObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "ProjectileObject";
	}

	void SetIsFlying(bool isFlying) { isFlying_ = isFlying; }
	void SetParam(const CraneProjectileParam& param) {
		param_ = param;
	}

	void OnCollisionEnter(Collider* other) override;

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// パラメータ
	CraneProjectileParam param_;

	// 飛んでいるか
	bool isFlying_ = false;
	bool isHit_	   = false;
	bool isParry_  = false;

	// 調整するパラメーター
	CalyxEngine::Vector3 velocity_;
	float targetTime_ = 0.0f;


};
