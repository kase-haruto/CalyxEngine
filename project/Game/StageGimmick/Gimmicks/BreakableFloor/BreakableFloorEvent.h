#pragma once

#include "Game\StageGimmick\Gimmicks\BreakableFloor\BreakableFloorObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// 壊れる床のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "BreakableFloorEvent")
class BreakableFloorEvent : public StageGimmickEventBase
{
public:

	BreakableFloorEvent() = default;
	BreakableFloorEvent(const std::string& name);
	~BreakableFloorEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "BreakableFloorEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<BreakableFloorObject>& target);

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
	/// 壊れる床イベントのパラメータ
	/// </summary>
	struct BreakableFloorEventParam : public CalyxEngine::SerializableObject {
		float yOffsetOnCreate = -0.5f;
		bool  disableAfterHit = true;

		BreakableFloorEventParam() {
			AddField("Y Offset On Create", yOffsetOnCreate).Category("BreakableFloorEvent").Range(-5.0f, 5.0f);
			AddField("Disable After Hit", disableAfterHit).Category("BreakableFloorEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "BreakableFloorEvent", "StageGimmick"};
		}
	};

private:

	// ターゲットの壊れる床オブジェクト
	std::weak_ptr<BreakableFloorObject> targetObject_;
	Guid targetObjectGuid_;

	// 調整項目
	BreakableFloorEventParam param_;
	bool hasSerializedParam_ = false;
};
