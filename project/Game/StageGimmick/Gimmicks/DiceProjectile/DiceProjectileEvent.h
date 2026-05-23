#pragma once

#include "Game\StageGimmick\Base\StageGimmickEventBase.h"
#include "Game\StageGimmick\Gimmicks\DiceProjectile\DiceSocketObject.h"
#include "Game\StageGimmick\Gimmicks\DiceProjectile\DiceProjectileObject.h"
#include "Game\StageGimmick\Parameters\StageGimmickParam.h"

#include "Engine/Foundation/Reflection/CalyxReflection.h"
#include "Engine/Foundation/Serialization/SerializableObject.h"

/// <summary>
/// サイコロが飛んでくるイベントクラス
/// </summary>
CALYX_OBJECT(Category = Event, DisplayName = "DiceProjectileEvent")
class DiceProjectileEvent : public StageGimmickEventBase
{
public:

	DiceProjectileEvent() = default;
	DiceProjectileEvent(const std::string& name);
	~DiceProjectileEvent() override = default;

	std::string_view GetObjectClassName() const override {
		return "DiceProjectileEvent";
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
	/// サイコロのイベント全体のパラメータ
	/// </summary>
	struct DiceProjectileEventParam : public CalyxEngine::SerializableObject {

		DiceProjectileParam param_;

		DiceProjectileEventParam() {
			AddField("scale", param_.scale).Category("DiceProjectileEvent");
			AddField("hitScale", param_.hitScale).Category("DiceProjectileEvent");
			AddField("rotateSpeed", param_.rotateSpeed).Category("DiceProjectileEvent");
			AddField("speed", param_.speed).Category("DiceProjectileEvent");
			AddField("parryDuration", param_.parryDuration).Category("DiceProjectileEvent");
			AddField("direction", param_.direction).Category("DiceProjectileEvent");
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, "DiceProjectileEvent", "StageGimmick"};
		}
	};

	/// <summary>
	/// サイコロのイベント個々のパラメータ
	/// </summary>
	struct AllDiceProjectileEventData : public CalyxEngine::SerializableObject {

		int objectCount = 1;
		int clearCount	= 1;
		std::string name_;

		AllDiceProjectileEventData() {
			AddField("ObjectCount", objectCount).ReadOnly();
			AddField("ClearCount", clearCount);
		}

		void SetEventName(const std::string& name) {
			name_ = name;
		}

		CalyxEngine::ParamPath GetParamPath() const override {
			return {CalyxEngine::ParamDomain::Game, name_ + "ObjectCount", "StageGimmick"};
		}
	};

	// 追加用関数
	void AddDiceProjectileObject();
	// 削除用関数
	void DeleteDroolObject();

private:
	
	// サイコロの収納箱
	std::weak_ptr<DiceSocketObject> socket_;
	Guid socketGuid_;

	// ターゲットのサイコロオブジェクト
	std::vector<std::weak_ptr<DiceProjectileObject>> targetObjects_;
	std::vector<Guid> targetObjectGuids_;

	// パラメータ
	DiceProjectileEventParam eventParam_;
	AllDiceProjectileEventData eventData_;
	bool hasSerializedEventParam_ = false;
	bool hasSerializedEventData_ = false;

};
