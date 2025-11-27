#include "BossStateMachine.h"
#include "../Base/BaseBossState.h"

// state
#include "../Idle/BossStateIdle.h"
#include "../Attack/BossStateAttack.h"

namespace  {
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
}

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
	switch (req.op) {
	case BaseBossState::TransitionRequest::Type::Change:
		ChangeState(req.nextType);
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
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期状態を設定
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::SetInitialState(BossStateType type) {
	stack_.clear();
	stack_.push_back(CreateState(type));
	stack_.back()->Enter();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::ChangeState(BossStateType nextType) {
	if (!stack_.empty()) {
		stack_.back()->Exit();
		stack_.pop_back();
	}
	stack_.push_back(CreateState(nextType));
	stack_.back()->Enter();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::PushState(BossStateType nextType) {
	if (!stack_.empty())
	stack_.back()->Exit();

	stack_.push_back(CreateState(nextType));
	stack_.back()->Enter();
}


/////////////////////////////////////////////////////////////////////////////////////////
//		状態の変更
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateMachine::PopState() {
	if (stack_.empty()) return;

	stack_.back()->Exit();
	stack_.pop_back();

	if (!stack_.empty())
		stack_.back()->Enter();
}

