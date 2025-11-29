#include "BossAnimController.h"
#include "../Details/BossAnimType.h"

BossAnimController::BossAnimController(AnimationModel* animModel) : animModel_(animModel) {}
BossAnimController::~BossAnimController() = default;

void BossAnimController::Initialize()const {
	// AnimationModel がロードした最初のアニメ名を取得
	std::string firstName = animModel_->GetCurrentAnimationName();

	// ゲーム側で指定した Idle ID を初期アニメに紐付ける
	animModel_->RegisterAnimation(static_cast<int16_t>(BossAnimType::Idle), firstName);
	animModel_->RegisterAnimation(static_cast<int16_t>(BossAnimType::AttackNormal),"bossAttackNormal");

	// ループ設定
	animModel_->SetLoop(static_cast<int16_t>(BossAnimType::Idle), true);
	animModel_->SetLoop(static_cast<int16_t>(BossAnimType::AttackNormal), false);
}

void BossAnimController::Register(int16_t id, const std::string& name,
								  const std::optional<std::string>& fileName)const {
	animModel_->RegisterAnimation(id, name, fileName);
}

void BossAnimController::Play(int16_t id, float blend) const {
	animModel_->Play(id, blend);
}

void BossAnimController::PlayOneShot(int16_t id, int16_t returnId, float blend) const {
	animModel_->PlayOneShot(id, returnId, blend);
}

void BossAnimController::SetLoop(int16_t id, bool isLoop) const {
	animModel_->SetLoop(id, isLoop);
}

std::string BossAnimController::GetCurrentAnimName() const {
	return animModel_->GetCurrentAnimationName();
}

bool BossAnimController::IsAnimFinished() const {
	return animModel_->IsAnimationFinished();
}
