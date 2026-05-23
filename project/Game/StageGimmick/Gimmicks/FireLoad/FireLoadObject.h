#pragma once

#include "Game\StageGimmick\Base\StageGimmickObjectBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 炎オブジェクト
/// </summary>
class FireLoadObject : public StageGimmickObjectBase {
public:
	FireLoadObject() = default;
	FireLoadObject(const std::string&		  modelName,
					std::optional<std::string> objectName = std::nullopt);
	~FireLoadObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "FireLoadObject";
	}

	void SetIsBurn(bool isBurn) {
		isBurn_ = isBurn;
	}

	void SetIsIgnition(bool isIgnition) {
		isIgnition_ = isIgnition;
	}

	void BurnActive();

	void SetParam(const FireLoadParam& param) {
		param_		  = param;
		runtimeParam_ = param;
	}

	void OnCollisionEnter(Collider* other) override;

protected:
	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

	void IgnitionUpdate(float dt);

private:
	// パラメータ
	FireLoadParam param_;
	// 実行中に変更されるパラメータ
	FireLoadParam	 runtimeParam_;
	CalyxEngine::Vector3 defaultScale_;

	float ignitionTime_ = 0.0f;

	bool isBurn_ = false;
	bool isIgnition_ = false;
};
