#include "TutorialEvent.h"
/* ========================================================================
/*      include space
/* ===================================================================== */
#include "Engine/Application/System/CalyxCore.h"
#include "Engine/Objects/Collider/BoxCollider.h"
#include <Engine/Application/UI/EngineUI/DebugTextManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Game/3dObject/Actor/Player/Player.h>
#include <algorithm>
#include <externals/imgui/imgui.h>

REGISTER_SCENE_OBJECT(TutorialEvent);

/////////////////////////////////////////////////////////////////////////
//		コンストラクタ / デストラクタ
/////////////////////////////////////////////////////////////////////////
TutorialEvent::TutorialEvent() {
	SceneObject::SetName("TutorialEvent", ObjectType::Event);
}

TutorialEvent::TutorialEvent(const std::string& name) : BaseEventObject(name) {
}

TutorialEvent::~TutorialEvent() = default;

/////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////

void TutorialEvent::Initialize() {
	BaseEventObject::Initialize();

	// 色を黄色に
	if(collider_) collider_->SetColor(CalyxMath::Vector3(1, 1, 0));

	// インスタンスの取得
	auto ctx = SceneContext::Current();
	if(ctx) {
		player_ = ctx->FindFirst<Player>().get();
	}

	param_.LoadParams();
	// tutorial用スプライトの初期化
	tutorialSprite_ = std::make_unique<Calyx2D::SpriteObject2d>();
	tutorialSprite_->Initialize("Textures/UI/R_Stick.png");
	// アニメーション設定
	Calyx2D::SpriteAnimation anim;
	anim.texturePath   = "Textures/UI/R_Stick.png";
	anim.division	   = {3, 1};
	anim.frameDuration = param_.animDuration_;
	anim.loop		   = true;

	tutorialSprite_->AddAnimation("reticleMove", anim);
	anim.texturePath   = "Textures/UI/RB.png";
	anim.frameDuration = param_.animDuration_ * 0.5f; // RBは少し速め
	tutorialSprite_->AddAnimation("pushRB", anim);
	tutorialSprite_->SetAnimation("reticleMove",false);
}

void TutorialEvent::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();

	if(state_ == State::LockOnPhase || state_ == State::AttackPhase) {
		// パラメータ適用
		tutorialSprite_->GetSprite()->SetSize(param_.spriteSize_);
		tutorialSprite_->SetPosition(param_.position_);
		tutorialSprite_->SetFrameDuration(param_.animDuration_);
		// スプライト描画
		tutorialSprite_->Update(dt);
	}

	auto								ctx = SceneContext::Current();
	std::vector<std::shared_ptr<Enemy>> currentEnemies;
	if(ctx) {
		currentEnemies = ctx->GetObjectLibrary()->FindByType<Enemy>();
	}

	// 距離ベースの進入検知 (衝突判定の重複問題への対策)
	if(player_) {
		CalyxMath::Vector3 pPos	  = player_->GetWorldPosition();
		CalyxMath::Vector3 myPos  = worldTransform_.GetWorldPosition();
		float			   dx	  = pPos.x - myPos.x;
		float			   dy	  = pPos.y - myPos.y;
		float			   dz	  = pPos.z - myPos.z;
		float			   distSq = dx * dx + dy * dy + dz * dz;

		// スケールを判定半径として使用 (最小 1.0)
		float radius	= (std::max)(worldTransform_.scale.x, 1.0f);
		isPlayerInside_ = (distSq < radius * radius);
	}

	// 状態遷移ロジック
	switch(state_) {
	case State::None:
		if(isPlayerInside_ && !currentEnemies.empty()) {
			// いずれかの敵が進入フェーズを終えているかチェック（画面内への到達待ち）
			bool anyArrived = false;
			for(auto& e : currentEnemies) {
				if(e && e->GetMovementController()->GetMode() != EnemyMovementController::Mode::Entrance) {
					anyArrived = true;
					break;
				}
			}

			if(anyArrived) {
				// チュートリアル対象の設定
				for(auto& e : currentEnemies) {
					if(!e) continue;
					// まだ Entrance 中の個体がいれば強制的に完了させる
					if(e->GetMovementController()->GetMode() == EnemyMovementController::Mode::Entrance) {
						e->GetMovementController()->StartActive();
					}
					e->SetLife(1);
					e->SetIsAlive(true);
				}

				param_.startTimeScale_		= currentTimeScale_;
				param_.targetTimeScale_	= 0.0f;
				timeScaleEaseTimer_ = 0.0f;
				state_				= State::LockOnPhase;
			}
		}
		break;

	case State::LockOnPhase:

		if(player_) {
			auto lockOn = player_->GetLockOn();
			if(lockOn) {
				const auto& locked = lockOn->GetLockedTargets();

				bool allLocked = true;
				for(auto& e : currentEnemies) {
					if(std::find(locked.begin(), locked.end(), e) == locked.end()) {
						allLocked = false;
						break;
					}
				}

				if(allLocked && !currentEnemies.empty()) {
					tutorialSprite_->SetAnimation("pushRB", true);
					state_ = State::AttackPhase;
				}
			}
		}else {
			if(ctx) {
				player_ = ctx->FindFirst<Player>().get();
			}
		}
		break;

	case State::AttackPhase:
		if(CalyxFoundation::Input::TriggerKey(DIK_SPACE) ||
		   CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::RB)) {

			// 敵を即座に殺すのではなく、時間を再開して弾が当たるのを待つ
			param_.startTimeScale_		= currentTimeScale_;
			param_.targetTimeScale_	= 1.0f;
			timeScaleEaseTimer_ = 0.0f;
			state_				= State::WaitingForDeath;
		}
		break;

	case State::WaitingForDeath: {
		bool allDeadOrGone = true;
		if(currentEnemies.empty()) {
			// 全員消えた（ライブラリから消えた）なら完了
			allDeadOrGone = true;
		} else {
			for(auto& e : currentEnemies) {
				if(e && e->GetIsAlive()) {
					allDeadOrGone = false;
					break;
				}
			}
		}

		if(allDeadOrGone) {
			state_ = State::Complete;
		}
	} break;

	case State::Complete:
		Destroy();
		break;
	}

	// 描画メッセージ
	ShowTutorialMsg();

	// デバッグ表示用情報の収集
	size_t enemyCount = currentEnemies.size();
	ShowDebugState(enemyCount);

	CalyxMath::Vector3	  worldPos = worldTransform_.GetWorldPosition();
	CalyxMath::Quaternion rot	   = worldTransform_.rotation;

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			collider_->Update(worldPos, rot);
			auto* box = dynamic_cast<BoxCollider*>(collider_.get());
			if(box) box->SetSize(worldTransform_.scale);
			// collider_->Draw();	// 線の描画は止めてモデルで代替
		}
	}

	if(model_) {
		model_->Update(dt);
		model_->SetIsDrawEnable(isDrawEnable_);
	}

	UpdateTimeScaleEasing(dt);
}
void TutorialEvent::DrawUISprite(class SpriteRenderer* spriteRenderer) const {
	// フェーズごとにUIスプライトを描画
	if(state_ == State::LockOnPhase|| state_ == State::AttackPhase) {
		tutorialSprite_->Draw(spriteRenderer);
	}
}

void TutorialEvent::UpdateTimeScaleEasing(float alwaysDt) {
	if(timeScaleEaseTimer_ < param_.timeScaleEaseDuration_) {
		timeScaleEaseTimer_ += alwaysDt;
		float t = (std::min)(timeScaleEaseTimer_ / param_.timeScaleEaseDuration_, 1.0f);
		currentTimeScale_ =
			CalyxEase::EaseLerp(param_.startTimeScale_, param_.targetTimeScale_, t, CalyxEase::EaseType::EaseOutSine);
		ClockManager::GetInstance()->SetTimeScale(currentTimeScale_);
	}
}

void TutorialEvent::ShowTutorialMsg() {
	if(state_ == State::None || state_ == State::Complete) return;

	if(state_ == State::LockOnPhase) {
		CalyxEditor::DebugTextManager::AddMessage("TUTORIAL: LOCK ON ALL ENEMIES",
												  "Move Reticle with WASD / Left Stick and Lock On automatically.",
												  ImVec4(1, 1, 0, 1));
	} else if(state_ == State::AttackPhase) {
		CalyxEditor::DebugTextManager::AddMessage("TUTORIAL: PRESS ATTACK BUTTON",
												  "Press SPACE / RB to defeat all locked enemies!",
												  ImVec4(1, 1, 0, 1));
	} else if(state_ == State::WaitingForDeath) {
		CalyxEditor::DebugTextManager::AddMessage("TUTORIAL: ATTACKING!",
												  "Waiting for impact...",
												  ImVec4(0, 1, 0, 1));
	}
}

void TutorialEvent::ShowDebugState([[maybe_unused]]size_t enemyCount) {

}

/////////////////////////////////////////////////////////////////////////
//		imgui/ui
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::ShowGui() {
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		param_.ShowGui();
	}
}

//	イベント発火時
// ///////////////////////////////////////////////////////////////////////
void TutorialEvent::OnCollisionEnter(Collider* other) {
	isPlayerInside_ = true;
	(void)other;
}

// ///////////////////////////////////////////////////////////////////////
//		イベント継続中の処理
// ///////////////////////////////////////////////////////////////////////
void TutorialEvent::OnCollisionStay(Collider* other) {
	(void)other;
}

// ///////////////////////////////////////////////////////////////////////
//		イベント終了時の処理
// ///////////////////////////////////////////////////////////////////////
void TutorialEvent::OnCollisionExit(Collider* other) {
	isPlayerInside_ = false;
	(void)other;
}

// ///////////////////////////////////////////////////////////////////////
//		パラメータ初期化
// ///////////////////////////////////////////////////////////////////////
TutorialEvent::TutorialEventParam::TutorialEventParam() {
	AddField("startTimeScale", startTimeScale_).Category("TimeScale");
	AddField("targetTimeScale", targetTimeScale_).Category("TimeScale");
	AddField("timeScaleEaseDuration", timeScaleEaseDuration_).Category("TimeScale");

	AddField("animDuration", animDuration_).Category("Animation");
	AddField("spriteSize", spriteSize_).Category("Animation");
	AddField("position", position_).Category("Animation");
}
CalyxEngine::ParamPath TutorialEvent::TutorialEventParam::GetParamPath() const {
	return{CalyxEngine::ParamDomain::Game,"TutorialEvent","Actor/Event"};
}