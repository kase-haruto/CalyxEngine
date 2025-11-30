#include "BossStateIdle.h"

#include "Game/3dObject/Actor/Boss/AI/BossAI.h"
#include "Game/3dObject/Actor/Boss/Anim/BossAnimController.h"
#include "Game/3dObject/Actor/Boss/Boss.h"
#include "Game/3dObject/Actor/Boss/Details/BossAnimType.h"

#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
BossStateIdle::BossStateIdle() {
	// タイプの設定
	BaseBossState::SetStatypeType(BossStateType::Idle);
}
BossStateIdle::~BossStateIdle() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateIdle::Initialize() {
	idleTime_ = 1.0f;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateIdle::Update(float dt) {
	idleTime_ -= dt;
	if (idleTime_ > 0.0f) return;

	// ボスのヒットフラグがたったらDamageリアクション
	if (owner_ && owner_->IsHit()) {
		RequestChange(BossStateType::Damage);
		return;
	}

	auto ai = owner_->GetAI();
	if (!ai) return;

	// 攻撃を決定
	auto atk = ai->DecideAttack(dt);
	if (!atk.has_value()) return;

	RequestChange(
		BossStateType::Attack,
		static_cast<int16_t>(atk.value())
	);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		GUI表示
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateIdle::ShowGui() {
	// 名前表示
	BaseBossState::ShowGui();

	GuiCmd::SliderFloat("idleTime", idleTime_, 0.1f, 10.0f);
}
void BossStateIdle::Enter() {
	if(!owner_) return;
	Initialize();
	// 待機アニメーション再生
	owner_->GetAnimator()->Play(static_cast<int16_t>(BossAnimType::Idle));
}