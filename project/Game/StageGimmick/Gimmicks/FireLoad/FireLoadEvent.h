#pragma once

#include <vector>

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\FireLoad\FireLoadObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// 炎のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "FireLoadEvent")
class FireLoadEvent : public StageGimmickEventBase {
public:
	FireLoadEvent() = default;
	FireLoadEvent(const std::string& name);
	~FireLoadEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "FireLoadEvent";
	}

	// 衝突開始時コールバック
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<FireLoadObject>& target);

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
	/// 炎のイベントのパラメータ
	/// </summary>
	struct FireLoadEventParam : public CalyxEngine::SerializableObject {

		FireLoadParam param_;
		float		  defaultZSpace = 15.0f;

		FireLoadEventParam() {
			AddField("Time", param_.time).Category("FireLoadEvent");
			AddField("Height", param_.burnHeight).Category("FireLoadEvent");
			AddField("Width", param_.width).Category("FireLoadEvent");
			AddField("Depth", param_.depth).Category("FireLoadEvent");
			AddField("DefaultZSpace", defaultZSpace).Category("FireLoadEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "FireLoadEvent", "StageGimmick"};
		}
	};

private:

	std::vector<std::weak_ptr<FireLoadObject>>	 targetObjects_;
	std::vector<Guid> targetObjectGuids_;

	FireLoadEventParam eventParam_;
	bool hasSerializedEventParam_ = false;
};
