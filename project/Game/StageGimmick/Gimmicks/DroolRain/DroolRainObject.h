#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// よだれ雨オブジェクト
/// </summary>
class DroolRainObject : public StageGimmickObjectBase 
{
public:

	DroolRainObject() = default;
	DroolRainObject(const std::string& modelName,
					std::optional<std::string> objectName = std::nullopt);
	~DroolRainObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "DroolRainObject";
	}

	void SetIsRaining(bool isRaining) {
		isRaining_ = isRaining;
	}

	void SetParam(const DroolRainParam& param) {
		param_ = param;
		runtimeParam_ = param;
	}

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// パラメータ
	DroolRainParam param_;
	// 実行中に変更されるパラメータ
	DroolRainParam runtimeParam_;
	float offsetY_ = 0.0f;
	float dropTime_ = 0.0f;
	CalyxEngine::Vector3 defaultScale_;

	// 雨が降っているか
	bool isRaining_ = false;
	bool isOnceSet_ = true; // 初期Offset用
};
