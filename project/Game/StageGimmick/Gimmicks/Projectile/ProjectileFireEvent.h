#pragma once

#include "Game\StageGimmick\Gimmicks\Projectile\ProjectileObject.h"
#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// 飛んでくる弾のイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "ProjectileFireEvent")
class ProjectileFireEvent : public StageGimmickEventBase 
{
public:

	ProjectileFireEvent() = default;
	ProjectileFireEvent(const std::string& name);
	~ProjectileFireEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "ProjectileFireEvent";
	}

	// ターゲットをセットする
	void SetTarget(const std::shared_ptr<ProjectileObject>& target);

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
	/// 飛んでくる弾イベントのパラメータ
	/// </summary>
	struct ProjectileFireEventParam : public CalyxEngine::SerializableObject {

		CraneProjectileParam param;

		ProjectileFireEventParam() {
			AddField("scale", param.scale).Category("ProjectileFireEvent");
			AddField("speed", param.speed).Category("ProjectileFireEvent");
			AddField("targetTime", param.targetTime).Category("ProjectileFireEvent");
			AddField("parryPositionY", param.parryPositionY).Category("ProjectileFireEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "ProjectileFireEvent", "StageGimmick"};
		}
	};

private:

	// ターゲットの地面スパイクオブジェクト
	std::weak_ptr<ProjectileObject> targetObject_;
	Guid targetObjectGuid_;

	// 調整するパラメーター
	ProjectileFireEventParam param_;

};
