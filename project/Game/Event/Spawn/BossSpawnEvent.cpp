#include "BossSpawnEvent.h"
/* ========================================================================
/*      include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Foundation/Clock/ClockManager.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/3dObject/Actor/Boss/Boss.h>

REGISTER_SCENE_OBJECT(BossSpawnEvent)

/////////////////////////////////////////////////////////////////////////
//		コンストラクタ / デストラクタ
/////////////////////////////////////////////////////////////////////////
BossSpawnEvent::BossSpawnEvent()
	: easeType_(CalyxEase::EaseType::Linear) {
	SceneObject::SetName("BossSpawnEvent",ObjectType::Event);
}
BossSpawnEvent::BossSpawnEvent(const std::string& name)
: BaseEventObject(name), easeType_(CalyxEase::EaseType::Linear) {
}
BossSpawnEvent::~BossSpawnEvent() =default;

/////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////
void BossSpawnEvent::Initialize() {
	BaseEventObject::Initialize();
	// 色を紫に
	if(collider_) collider_->SetColor(CalyxMath::Vector4(1, 0, 1));
}

void BossSpawnEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt);
	ZoomCameraForBoss(dt);
}

/////////////////////////////////////////////////////////////////////////
//		常時更新
/////////////////////////////////////////////////////////////////////////
void BossSpawnEvent::OnCollisionEnter(Collider* other) {
	(void)other;
	// プレイヤーが入ったらスポーン
	auto ctx = SceneContext::Current();
	if(!ctx || !ctx->GetObjectLibrary()) {
		return;
	}

	auto spawners = ctx->GetObjectLibrary()->FindByType<BossSpawner>();
	if(spawners.empty()) {
		return;
	}

	const Actor* target = nullptr;
	if(auto player = ctx->FindFirst<Player>()) {
		target = player.get();
	}

	bool anySpawned = false;
	for(const auto& spawner : spawners) {
		if(!spawner || spawner->WasSpawned()) {
			continue;
		}
		spawner->SetPlayerTransform(target);
		spawner->Spawn();
		anySpawned = true;
	}

	if (anySpawned) {
		auto bosses = ctx->GetObjectLibrary()->FindByType<Boss>();
		if (!bosses.empty()) {
			wBoss_ = bosses.front();

			auto cam = CameraManager::GetMain3d();
			if (cam) {
				auto boss = wBoss_.lock();
				if (boss) {

					startPos_ = cam->GetWorldTransform().translation;
					startRot_ = cam->GetWorldTransform().rotation;

					// Bossの正面にカメラを移動させる
					CalyxMath::Vector3 bossPos = boss->GetWorldTransform().GetWorldPosition();

					// ボスの向いている方向を取得 (m[2] is Forward in World Matrix)
					const auto& m = boss->GetWorldTransform().matrix.world;
					CalyxMath::Vector3 bossForward(m.m[2][0], m.m[2][1], m.m[2][2]);
					bossForward = bossForward.Normalize();

					// Bossの正面 15m, 高さ 5m
					endPos_ = bossPos + (bossForward * 5.0f) + CalyxMath::Vector3(20, 10.0f, 200);

					// Bossの頭を見る
					CalyxMath::Vector3 at = bossPos + CalyxMath::Vector3(0, 2.0f, 0);
					CalyxMath::Vector3 up(0, 1, 0);

					endRot_ = CalyxMath::Quaternion::LookAt(endPos_, at, up);

					directionState_ = DirectionState::ZoomIn;
					currentTimer_ = 0.0f;

					// ゲーム時間を止める
					ClockManager::GetInstance()->SetTimeScale(0.0f);
				}
			}
		}
	}
}
void BossSpawnEvent::DerivativeGui() {
	BaseEventObject::DerivativeGui();
}

/////////////////////////////////////////////////////////////////////////
//		ズーム
/////////////////////////////////////////////////////////////////////////
void BossSpawnEvent::ZoomCameraForBoss(float dt) {
if (directionState_ == DirectionState::Idle || directionState_ == DirectionState::Finished) {
		return;
	}

	auto cam = CameraManager::GetMain3d();
	if (!cam) return;

	currentTimer_ += dt;
	float t = 0.0f;
	float easedT = 0.0f;
	// キャスト
	CalyxEase::EaseType ease = static_cast<CalyxEase::EaseType>(data_.easeType_);

	switch (directionState_) {
	case DirectionState::ZoomIn:
		t = currentTimer_ / data_.zoomInDuration_;
		if (t >= 1.0f) {
			t = 1.0f;
			directionState_ = DirectionState::Stay;
			currentTimer_ = 0.0f;
		}
		easedT = CalyxEase::ApplyEase(ease, t);

		// 補間
		cam->GetWorldTransform().translation = CalyxMath::Vector3::Lerp(startPos_, endPos_, easedT);
		cam->GetWorldTransform().rotation = CalyxMath::Quaternion::Slerp(startRot_, endRot_, easedT);

		// ZoomIn完了時、Stayへ移行（上でセット済み）
		if (directionState_ == DirectionState::Stay) {
			// ZoomOutの為の準備
			// 戻りの移動先をstartPos_(元の位置)にする
			// 現在位置(endPos_)から、元の位置(startPos_)へ戻る
			// 変数をスワップしてZoomOutで同じロジックを使えるようにする
			// startPos_ <-> endPos_
			CalyxMath::Vector3 tempPos = startPos_;
			startPos_ = endPos_;
			endPos_ = tempPos;

			CalyxMath::Quaternion tempRot = startRot_;
			startRot_ = endRot_;
			endRot_ = tempRot;
		}
		break;

	case DirectionState::Stay:
		if (currentTimer_ >= data_.stayDuration_) {
			directionState_ = DirectionState::ZoomOut;
			currentTimer_ = 0.0f;
		}
		break;

	case DirectionState::ZoomOut:
		t = currentTimer_ / data_.zoomOutDuration_;
		if (t >= 1.0f) {
			t = 1.0f;
			directionState_ = DirectionState::Finished;
			// 追従再開
			cam->EnableFollow(true);
			// ゲーム時間を再開
			ClockManager::GetInstance()->SetTimeScale(1.0f);
		}
		easedT = CalyxEase::ApplyEase(ease, t);

		cam->GetWorldTransform().translation = CalyxMath::Vector3::Lerp(startPos_, endPos_, easedT);
		cam->GetWorldTransform().rotation = CalyxMath::Quaternion::Slerp(startRot_, endRot_, easedT);
		break;
	}
}

BossSpawnEvent::BossSpawnEventData::BossSpawnEventData() {
	AddField("zoomInDuration", zoomInDuration_);
	AddField("stayDuration", stayDuration_);
	AddField("zoomOutDuration", zoomOutDuration_);
	AddField("easeType", easeType_);
}

CalyxEngine::ParamPath BossSpawnEvent::BossSpawnEventData::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game,"BossSpawnEvent", "Event"};
}