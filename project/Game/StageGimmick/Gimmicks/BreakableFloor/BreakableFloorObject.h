#pragma once

#include "Game/StageGimmick/Base/StageGimmickObjectBase.h"

/// <summary>
/// 壊れる床オブジェクトのクラス
/// </summary>
class BreakableFloorObject : public StageGimmickObjectBase
{
public:

	BreakableFloorObject() = default;
	BreakableFloorObject(const std::string&	modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~BreakableFloorObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "BreakableFloorObject";
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
