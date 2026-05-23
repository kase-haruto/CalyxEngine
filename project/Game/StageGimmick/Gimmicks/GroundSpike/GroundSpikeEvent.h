#pragma once

#include "Game\StageGimmick\Gimmicks\GroundSpike\GroundSpikeObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Prediction\PredictionCircle.h"

#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// 地面から生えてくる歯のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "GroundSpikeEvent")
class GroundSpikeEvent : public StageGimmickEventBase 
{
public:

	GroundSpikeEvent() = default;
	GroundSpikeEvent(const std::string& name);
	~GroundSpikeEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "GroundSpikeEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<GroundSpikeObject>& target);

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;

protected:

	// 初期化
	void EventInitialize() override;

	// 更新
	void EventUpdate(float dt) override;

	// gui
	void DerivativeGui() override;
	void ApplyDerivedConfigFromJson(const nlohmann::json& root, const nlohmann::json* derived) override;
	void ExtractDerivedConfigToJson(nlohmann::json& root, nlohmann::json& derived) const override;
	void RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) override;

private:
	/// <summary>
	/// 地面から生えてくる歯イベント全体のパラメータ
	/// </summary>
	struct GroundSpikeEventParam : public CalyxEngine::SerializableObject {

		GroundSpikeParam param_;

		GroundSpikeEventParam() {
			AddField("popUpSpeed", param_.popUpSpeed).Category("GroundSpikeEvent");
			AddField("popUpHeight", param_.popUpHeight).Category("GroundSpikeEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "GroundSpikeEvent", "StageGimmick"};
		}
	};

private:

	// ターゲットの地面スパイクオブジェクト
	std::weak_ptr<GroundSpikeObject> targetObject_;
	Guid targetObjectGuid_;
	std::weak_ptr<PredictionCircle> predictionObject_;
	Guid predictionObjectGuid_;


	GroundSpikeEventParam eventParam_;
	bool hasSerializedEventParam_ = false;

};
