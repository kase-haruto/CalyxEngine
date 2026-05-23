#pragma once

#include"Game\StageGimmick\Gimmicks\BreakableWall\BreakableWallObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// 壊れる壁のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "BreakableWallEvent")
class BreakableWallEvent : public StageGimmickEventBase
{
public:

	BreakableWallEvent() = default;
	BreakableWallEvent(const std::string& name);
	~BreakableWallEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "BreakableWallEvent";
	}

	void SetTarget(const std::shared_ptr<BreakableWallObject>& target);

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
	/// 壊れる壁イベントのパラメータ
	/// </summary>
	struct BreakableWallEventParam : public CalyxEngine::SerializableObject {
		float yOffsetOnCreate = -0.5f;
		bool  disableAfterHit = true;

		BreakableWallEventParam() {
			AddField("Y Offset On Create", yOffsetOnCreate).Category("BreakableWallEvent").Range(-5.0f, 5.0f);
			AddField("Disable After Hit", disableAfterHit).Category("BreakableWallEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "BreakableWallEventParam", "StageGimmick"};
		}
	};

private:

	// ターゲットの壊れる壁オブジェクト
	std::weak_ptr<BreakableWallObject> targetObject_;
	Guid targetObjectGuid_;

	// 調整項目
	BreakableWallEventParam param_;

};
