#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Game/3dObject/Actor/Boss/Spawner/BossSpawner.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

// 演出の状態
enum class DirectionState {
	Idle,
	ZoomIn,
	Stay,
	Finished,
	ZoomOut,
};

/*-----------------------------------------------------------------------------------------
* BossSpawnEvent
* - Bossをスポーンさせるイベント
* - ボスをスポーンさせたときの演出を入れるためのイベント
*---------------------------------------------------------------------------------------*/
class BossSpawnEvent final
: public BaseEventObject {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BossSpawnEvent();
	BossSpawnEvent(const std::string& name);
	~BossSpawnEvent() override;

	// 初期化
	void Initialize() override;

	void AlwaysUpdate(float dt) override;

	// 発火時処理
	void OnCollisionEnter(Collider* other) override;

	// debugUI
	void DerivativeGui() override;
	
	// クラス名取得
	std::string_view GetTypeName() const override{return "BossSpawnEvent";};

private:
	void ZoomCameraForBoss(float dt);


private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	DirectionState directionState_ = DirectionState::Idle; // 演出の状態
	CalyxEase::EaseType easeType_;
	std::weak_ptr<Boss> wBoss_;

	// 演出用
	float currentTimer_ = 0.0f;
	CalyxMath::Vector3 startPos_;
	CalyxMath::Quaternion startRot_;
	CalyxMath::Vector3 endPos_;
	CalyxMath::Quaternion endRot_;

	struct BossSpawnEventData : public CalyxEngine::SerializableObject {
		BossSpawnEventData();
		CalyxEngine::ParamPath GetParamPath() const override;

		float zoomInDuration_  = 1.0f; // ズームインの時間
		float stayDuration_    = 1.0f;
		float zoomOutDuration_ = 1.0f; // ズームアウトの時間
		int32_t easeType_       = static_cast<int32_t>(CalyxEase::EaseType::Linear); // イージングタイプ
	} data_;
};