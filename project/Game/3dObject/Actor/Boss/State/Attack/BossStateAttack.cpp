#include "BossStateAttack.h"

#include "Game/3dObject/Actor/Boss/Anim/BossAnimController.h"
#include "Game/3dObject/Actor/Boss/Attack/BossHomingSpreadShoot.h"
#include "Game/3dObject/Actor/Boss/Attack/BossLaser.h"
#include "Game/3dObject/Actor/Boss/Attack/BossNormalShoot.h"
#include "Game/3dObject/Actor/Boss/Boss.h"
#include "Game/3dObject/Actor/Boss/Details/BossAnimType.h"
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>


/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
BossStateAttack::BossStateAttack() {
	// タイプの設定
	BaseBossState::SetStatypeType(BossStateType::Attack);

	// 攻撃テーブルの初期化
	attacks_[BossAttackType::NormalShoot] = std::make_unique<BossNormalShoot>();
	attacks_[BossAttackType::Punch]		  = std::make_unique<BossHomingSpreadShoot>();
	attacks_[BossAttackType::Laser]		  = std::make_unique<BossLaser>();

	attackAnimTable_ = {
		{BossAttackType::NormalShoot, BossAnimType::AttackNormal},
		{BossAttackType::Punch, BossAnimType::Punch},
		{BossAttackType::Laser, BossAnimType::Laser},
	};
}

BossStateAttack::~BossStateAttack() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::Update(float dt) {
	// クールダウン中
	if(repeatCooldownTimer_ > 0.0f) {
		repeatCooldownTimer_ -= dt;
		if(repeatCooldownTimer_ <= 0.0f) {
			// クールダウン明けに再実行
			// 常に UI で選択されている攻撃タイプを反映させる
			SetTransitionParam(owner_->GetForcedAttackType());
			Enter();
		}
		return;
	}

	timer_ += dt;

	if(owner_->GetAnimator()->IsAnimFinished()) {
		// デバッグループが有効な場合はクールダウンを挟んで再実行
		if(owner_->IsDebugLoopEnabled()) {
			repeatCooldownTimer_ = param_.repeatColldownLimit;
			return;
		}
		RequestChange(BossStateType::Idle);
		return;
	}

	if(timer_ >= param_.maxAttackTime) {
		if(owner_->IsDebugLoopEnabled()) {
			repeatCooldownTimer_ = param_.repeatColldownLimit;
			return;
		}
		RequestChange(BossStateType::Idle);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態に入るときの処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::Enter() {
	timer_				 = 0.0f;
	repeatCooldownTimer_ = 0.0f;

	attackType_ = static_cast<BossAttackType>(GetTransitionParam());

	auto it = attackAnimTable_.find(attackType_);
	if(it != attackAnimTable_.end()) {
		owner_->GetAnimator()->Play(static_cast<int16_t>(it->second));
	}

	ExecuteAttack();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ表示
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::ShowGui() {
	BaseBossState::ShowGui();
	const auto attackStr = CalyxUtil::EnumConverter<BossAttackType>::ToString(attackType_);
	ImGui::Text("Attack Type: %.*s",
				static_cast<int>(attackStr.size()),
				attackStr.data());

	// 現在の攻撃用の GUI 表示
	auto it = attacks_.find(attackType_);
	if(it != attacks_.end()) {
		IBossAttack* atk = it->second.get();
		atk->ShowGui();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		攻撃を実行
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::ExecuteAttack() const {
	// AttackType に対応する攻撃クラスを探す
	auto it = attacks_.find(attackType_);
	if(it == attacks_.end()) return;

	IBossAttack* atk = it->second.get();

	// Boss 本体から Shooter を取得
	BossShootingController* shooter = owner_->GetShootController();
	if(!shooter) return;
	// 攻撃実行
	atk->Execute(*owner_, *shooter);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		パラメータ
/////////////////////////////////////////////////////////////////////////////////////////
BossStateAttack::BossAttackParam::BossAttackParam() {
	AddField("maxAttackTime", maxAttackTime).Category("Attack").Range(0.1f, 30.0f);
	AddField("repeatColldownLimit", repeatColldownLimit).Category("Attack").Range(0.0f, 10.0f);
}

CalyxEngine::ParamPath BossStateAttack::BossAttackParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game,"BossAttack","Actor/Boss/State"};
}