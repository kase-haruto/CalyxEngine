#pragma once
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Base\GeneralObject.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileObject.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileTarget.h"
#include "Game\StageGimmick\Gimmicks\BellProjectile\BellProjectileDoor.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"



CALYX_OBJECT(Category = Event, DisplayName = "BellProjectileEvent")
class BellProjectileEvent : public StageGimmickEventBase {
public:
	BellProjectileEvent() = default;
	BellProjectileEvent(const std::string& name);
	~BellProjectileEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "BellProjectileEvent";
	}

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
	/// 鐘イベント全体のパラメータ
	/// </summary>
	struct BellProjectileEventParam : public CalyxEngine::SerializableObject {

		BellProjectileParam param_;
		BellProjectileTargetParam targetParam_;
		BellProjectileDoorParam doorParam_;

		BellProjectileEventParam() {
			AddField("scale", param_.scale).Category("BellProjectileEvent");
			AddField("hitScale", param_.hitScale).Category("BellProjectileEvent");
			AddField("direction", param_.direction).Category("BellProjectileEvent");
			AddField("speed", param_.speed).Category("BellProjectileEvent");
			AddField("parryDuration", param_.parryDuration).Category("BellProjectileEvent");

			AddField("targetScale", targetParam_.scale).Category("BellProjectileTarget");
			AddField("targetHitScale", targetParam_.hitScale).Category("BellProjectileTarget");
			AddField("targetMoveSpeed", targetParam_.moveSpeed).Category("BellProjectileTarget");

			AddField("doorScale", doorParam_.scale).Category("BellProjectileDoor");
			AddField("doorSpeed", doorParam_.speed).Category("BellProjectileDoor");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "BellProjectileEvent", "StageGimmick"};
		}
	};


private:
	// 鐘
	std::weak_ptr<BellProjectileTarget> bell_;
	Guid							bellGuid_;

	// 撞木
	std::weak_ptr<BellProjectileObject> targetObject_;
	Guid								targetObjectGuid_;

	// 扉
	std::weak_ptr<BellProjectileDoor> doorL_;
	Guid							doorLGuid_;
	std::weak_ptr<BellProjectileDoor> doorR_;
	Guid							doorRGuid_;

	// 建物
	std::weak_ptr<GeneralObject> gate_;
	Guid						 gateGuid_;

	// パラメータ
	BellProjectileEventParam   eventParam_;
	bool					   hasSerializedEventParam_ = false;
};