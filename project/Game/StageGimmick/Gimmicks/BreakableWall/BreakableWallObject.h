#pragma once

#include "Game/StageGimmick/Base/StageGimmickObjectBase.h"

/// <summary>
/// 壊れる壁のオブジェクトクラス
/// </summary>
class BreakableWallObject : public StageGimmickObjectBase
{
public:

	BreakableWallObject() = default;
	BreakableWallObject(const std::string& modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~BreakableWallObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "BreakableWallObject";
	}

	bool IsBroken() const { return isBroken_; }

	// 壊れる床を壊す
	void Break();

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// 壊れているか
	bool isBroken_ = false;

};
