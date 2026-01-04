#include "BossStateMachine.h"
#include "../Base/BaseBossState.h"

// state
#include "../Attack/BossStateAttack.h"
#include "../Idle/BossStateIdle.h"

// c++
#include <externals/imgui/imgui.h>

namespace {
std::unique_ptr<BaseBossState> CreateState(BossStateType type) {
	switch(type) {
	case BossStateType::Idle:
		return std::make_unique<BossStateIdle>();
	case BossStateType::Attack:
		return std::make_unique<BossStateAttack>();
	default:
		return nullptr;
	}
}
} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
BossStateMachine::BossStateMachine()  = default;
BossStateMachine::~BossStateMachine() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::Update(float dt) {
	if (stack_.empty()) return;

	BaseBossState* cur = stack_.back().get();
	cur->Update(dt);

	auto req = cur->GetTransitionRequest();
	if (req.op != BaseBossState::TransitionRequest::Type::None) {

		HandleTransition(req);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態遷移の処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::HandleTransition(const BaseBossState::TransitionRequest& req) {
	switch(req.op) {
	case BaseBossState::TransitionRequest::Type::Change:
		ChangeState(req.nextType, req.param);
		break;
	case BaseBossState::TransitionRequest::Type::Push:
		PushState(req.nextType);
		break;
	case BaseBossState::TransitionRequest::Type::Pop:
		PopState();
		break;
	default:
		break;
	}

	if (!stack_.empty()) {
		stack_.back()->ResetRequest();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期状態を設定
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::SetInitialState(BossStateType type) {
	stack_.clear();
	stack_.push_back(CreateState(type));
	stack_.back()->SetOwner(owner_);
	stack_.back()->Enter();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::ShowGui() {
	if(stack_.empty()) {
		ImGui::Text("No state (stack empty)");
		return;
	}

	ImGui::Text("=== Boss State Debug ===");
	ImGui::Separator();

	// 現在のステート名を表示
	ImGui::Text("Current: %s", stack_.back()->GetStateName().c_str());

	ImGui::Spacing();
	ImGui::Text("Change State:");
	ImGui::Separator();

	// --- ChangeState Buttons ---
	if(ImGui::Button("Idle")) {
		ChangeState(BossStateType::Idle);
	}
	ImGui::SameLine();
	if(ImGui::Button("Attack")) {
		ChangeState(BossStateType::Attack);
	}

	ImGui::Spacing();
	ImGui::Text("Push State:");
	ImGui::Separator();

	// --- PushState Buttons ---
	if(ImGui::Button("Push Idle")) {
		PushState(BossStateType::Idle);
	}
	ImGui::SameLine();
	if(ImGui::Button("Push Attack")) {
		PushState(BossStateType::Attack);
	}

	ImGui::Spacing();
	ImGui::Text("Pop State:");
	ImGui::Separator();

	// --- PopState Button ---
	if(ImGui::Button("Pop")) {
		PopState();
	}

	// スタック状況も表示
	ImGui::Spacing();
	ImGui::Text("State Stack:");
	for(int i = 0; i < (int)stack_.size(); i++) {
		ImGui::BulletText("[%d] %s", i, stack_[i]->GetStateName().c_str());
	}

	// 各ステートの独自GUIも呼ぶ
	ImGui::Separator();
	ImGui::Text("=== State Internal GUI ===");

	if(stack_.empty()) return;
	stack_.back()->ShowGui();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ボスを設定
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::SetOwner(Boss* owner) {
	owner_ = owner;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::ChangeState(BossStateType nextType, int16_t param) {
	// 現在のステートを削除
	if (!stack_.empty()) {
		stack_.back()->Exit();
		stack_.pop_back();
	}

	// 新しいステート作成
	auto st = CreateState(nextType);
	st->SetOwner(owner_);
	st->SetTransitionParam(param);

	// ここで push してから Enter を呼ぶ
	stack_.push_back(std::move(st));
	stack_.back()->Enter();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::PushState(BossStateType nextType) {
	if(!stack_.empty())
		stack_.back()->Exit();

	auto st = CreateState(nextType);
	st->SetOwner(owner_);

	stack_.push_back(std::move(st));
	stack_.back()->Enter();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::PopState() {
	if(stack_.empty()) return;

	stack_.back()->Exit();
	stack_.pop_back();

	if(!stack_.empty())
		stack_.back()->Enter();
}
