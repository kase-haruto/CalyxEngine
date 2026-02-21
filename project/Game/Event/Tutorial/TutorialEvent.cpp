#include "TutorialEvent.h"
/* ========================================================================
/*      include space
/* ===================================================================== */
#include "Engine/Application/System/CalyxCore.h"
#include "Engine/Objects/Collider/BoxCollider.h"
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

REGISTER_SCENE_OBJECT(TutorialEvent);

/////////////////////////////////////////////////////////////////////////
//		コンストラクタ / デストラクタ
/////////////////////////////////////////////////////////////////////////
TutorialEvent::TutorialEvent() {
	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider");   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
}

TutorialEvent::TutorialEvent(const std::string& name) {
	SceneObject::SetName(name, ObjectType::Event);

	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider");   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
}

TutorialEvent::~TutorialEvent() = default;

/////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Game/3dObject/Actor/Player/Player.h>
#include <algorithm>
#include <externals/imgui/imgui.h>

void TutorialEvent::Initialize() {
	// 色を黄色に設定する
	collider_->SetColor(CalyxMath::Vector3(1, 1, 0));

	// インスタンスの取得
	auto ctx = SceneContext::Current();
	if(ctx) {
		player_	 = ctx->FindFirst<Player>().get();
		enemies_ = ctx->FindFirst<EnemyCollection>().get();
	}
}

void TutorialEvent::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();

	// 状態遷移ロジック
	switch(state_) {
	case State::None:
		if(isPlayerInside_ && enemies_ && !enemies_->GetEnemies().empty()) {
			ClockManager::GetInstance()->SetTimeScale(0.0f);
			state_ = State::LockOnPhase;
		}
		break;

	case State::LockOnPhase:
		if(player_) {
			auto lockOn = player_->GetLockOn();
			if(lockOn && enemies_) {
				const auto& locked	   = lockOn->GetLockedTargets();
				const auto& allEnemies = enemies_->GetEnemies();

				bool allLocked = true;
				for(auto& e : allEnemies) {
					if(std::find(locked.begin(), locked.end(), e) == locked.end()) {
						allLocked = false;
						break;
					}
				}

				if(allLocked && !allEnemies.empty()) {
					state_ = State::AttackPhase;
				}
			}
		}
		break;

	case State::AttackPhase:
		if(CalyxFoundation::Input::TriggerKey(DIK_SPACE) ||
		   CalyxFoundation::Input::TriggerGamepadButton(CalyxFoundation::PadButton::RB)) {

			if(enemies_) {
				for(auto& e : enemies_->GetEnemies()) {
					e->SetLife(0);
				}
			}
			ClockManager::GetInstance()->SetTimeScale(1.0f);
			state_ = State::Complete;
		}
		break;

	case State::Complete:
		Destroy();
		break;
	}

	// 描画メッセージ
	ShowTutorialMsg();
	ShowDebugState();

	CalyxMath::Vector3	  worldPos = worldTransform_.GetWorldPosition();
	CalyxMath::Quaternion rot	   = worldTransform_.rotation;

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			collider_->Update(worldPos, rot);
			auto* box = dynamic_cast<BoxCollider*>(collider_.get());
			if(box) box->SetSize(worldTransform_.scale);
			collider_->Draw();
		}
	}
}

void TutorialEvent::ShowTutorialMsg() {
	if(state_ == State::None || state_ == State::Complete) return;

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
							 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
							 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

	const ImGuiViewport* viewport  = ImGui::GetMainViewport();
	ImVec2				 work_pos  = viewport->WorkPos;
	ImVec2				 work_size = viewport->WorkSize;
	ImVec2				 window_pos;
	window_pos.x = work_pos.x + work_size.x * 0.5f;
	window_pos.y = work_pos.y + work_size.y * 0.2f;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowBgAlpha(0.7f);

	if(ImGui::Begin("TutorialMessage", nullptr, flags)) {
		ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]); // デフォルトフォント
		if(state_ == State::LockOnPhase) {
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "TUTORIAL: LOCK ON ALL ENEMIES");
			ImGui::Text("Move Reticle with WASD / Left Stick and Lock On automatically.");
		} else if(state_ == State::AttackPhase) {
			ImGui::TextColored(ImVec4(1, 1, 0, 1), "TUTORIAL: PRESS ATTACK BUTTON");
			ImGui::Text("Press SPACE / RB to defeat all locked enemies!");
		}
		ImGui::PopFont();
	}
	ImGui::End();
}

void TutorialEvent::ShowDebugState() {
	ImGui::Begin("Tutorial Debug");
	ImGui::Text("State: %d", (int)state_);
	ImGui::Text("Player Inside: %s", isPlayerInside_ ? "Yes" : "No");
	ImGui::Text("Player found: %s", player_ ? "Yes" : "No");
	ImGui::Text("Enemies found: %s", enemies_ ? "Yes" : "No");
	if(enemies_) {
		ImGui::Text("Enemy count: %zu", enemies_->GetEnemies().size());
	}
	ImGui::End();
}

/////////////////////////////////////////////////////////////////////////
//		imgui/ui
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::ShowGui() {
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		// NOTE: イベントのパラメータをここで表示
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
