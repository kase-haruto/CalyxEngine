#pragma once
#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"


class GeneralObject : public StageGimmickObjectBase {
public:
	GeneralObject() = default;
	GeneralObject(const std::string&		 modelName,
					   std::optional<std::string> objectName = std::nullopt);
	~GeneralObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "GeneralObject";
	}

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;
};