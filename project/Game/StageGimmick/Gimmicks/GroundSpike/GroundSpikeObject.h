#pragma once

#include "Game/StageGimmick/Base/StageGimmickObjectBase.h"

#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

/// <summary>
/// 地面から生えてくる歯のオブジェクトクラス
/// </summary>
class GroundSpikeObject : public StageGimmickObjectBase
{
public:

	GroundSpikeObject() = default;
	GroundSpikeObject(const std::string& modelName,
					  std::optional<std::string> objectName = std::nullopt);
	~GroundSpikeObject() override = default;

	std::string_view GetObjectClassName() const override {
		return "GroundSpikeObject";
	}

	bool IsSpike() const { return isSpike_; }

	// 飛び出す
	void Spike();

	void OnCollisionEnter(Collider* other) override;

	void SetParam(const GroundSpikeParam& param) {
		param_ = param;
	}

protected:

	// 初期化
	void ObjectInitialize() override;

	// 更新
	void ObjectUpdate(float dt) override;

private:

	// 飛び出しているか
	bool isSpike_ = false;
	bool isBreak_ = false;
	bool isBuried_ = false;

	float time_ = 0.0f;

	// 調整するパラメーター
	float spikeTime_ = 1.0f;
	float spikeAimY_ = 0.5f;

	GroundSpikeParam param_;

};
