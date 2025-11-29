#include "BossStateAttack.h"

#include "Game/3dObject/Actor/Boss/Anim/BossAnimController.h"
#include "Game/3dObject/Actor/Boss/Attack/BossNormalShoot.h"
#include "Game/3dObject/Actor/Boss/Boss.h"
#include "Game/3dObject/Actor/Boss/Details/BossAnimType.h"

namespace {

/**
 * \brief 攻撃タイプを文字列に変換
 * \param type 攻撃タイプ
 * \return 文字列
 */
std::string_view AttackTypeToString(BossAttackType type) {
	using namespace std::literals;

	switch(type) {
	case BossAttackType::NormalShoot:
		return "NormalShoot"sv;
	case BossAttackType::Punch:
		return "Punch"sv;
	case BossAttackType::Laser:
		return "Laser"sv;
	}
	return "Unknown"sv;
}

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////////////////////
BossStateAttack::BossStateAttack() {
	// タイプの設定
	BaseBossState::SetStatypeType(BossStateType::Attack);

	// 攻撃テーブルの初期化
	attacks_[BossAttackType::NormalShoot] = std::make_unique<BossNormalShoot>();
	attacks_[BossAttackType::Punch]       = std::make_unique<BossNormalShoot>();

	attackAnimTable_ = {
		{BossAttackType::NormalShoot, BossAnimType::AttackNormal},
		{BossAttackType::Punch, BossAnimType::Punch},
		{BossAttackType::Laser, BossAnimType::AttackNormal},
	};
}

BossStateAttack::~BossStateAttack() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::Update(float dt) {
	(void)dt;

	// アニメーション終了で次の状態へ移行
	if(owner_->GetAnimator()->IsAnimFinished()) {
		BaseBossState::RequestChange(BossStateType::Idle);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		状態に入るときの処理
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::Enter() {
	attackType_ = static_cast<BossAttackType>(GetTransitionParam());

	auto it = attackAnimTable_.find(attackType_);
	if(it != attackAnimTable_.end()) {
		owner_->GetAnimator()->Play(static_cast<int16_t>(it->second), false);
	}

	ExecuteAttack();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ表示
/////////////////////////////////////////////////////////////////////////////////////////
void BossStateAttack::ShowGui() {
	BaseBossState::ShowGui();
	const auto attackStr = AttackTypeToString(attackType_);
	ImGui::Text("Attack Type: %.*s",
				static_cast<int>(attackStr.size()),
				attackStr.data());
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