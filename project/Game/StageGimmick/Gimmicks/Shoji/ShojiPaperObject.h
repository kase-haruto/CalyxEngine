#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"

class ShojiPaperObject : public StageGimmickObjectBase
{
public:

	ShojiPaperObject() = default;
	ShojiPaperObject(const std::string& modelName,
						 std::optional<std::string> objectName = std::nullopt);
	~ShojiPaperObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "ShojiPaperObject";
	}

	const bool GetIsGetPosition() const { return isGetPosition_; }
	void SetIsGetPosition(bool flag) {
		isGetPosition_ = flag;
	}

protected:
	
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	bool isGetPosition_ = false;
};
