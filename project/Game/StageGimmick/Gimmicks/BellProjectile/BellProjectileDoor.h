#pragma once
#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

class BellProjectileTarget;

/// <summary>
/// 鐘イベントの扉
/// </summary>
class BellProjectileDoor : public StageGimmickObjectBase {
public:
	BellProjectileDoor() = default;
	BellProjectileDoor(const std::string&			modelName,
					 std::optional<std::string> objectName = std::nullopt);
	~BellProjectileDoor() override = default;

	std::string_view GetObjectClassName() const override {
		return "BellProjectileDoor";
	}

	void SetParam(const BellProjectileDoorParam& param) {
		param_ = param;
	}
	void SetTarget(BellProjectileTarget* target) {
		target_ = target;
	}

	// 0:左、1:右
	void SetLR(int lr);

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:
	// 鐘のポインタ
	BellProjectileTarget* target_ = nullptr;
	// パラメータ
	BellProjectileDoorParam param_;

	int lr_ = 0;
	// 開ききったか
	bool isOpened_ = false;
	// 開くアニメーション用
	float openRate_ = 0.0f;
	CalyxEngine::Vector3 startPos_{};
};
