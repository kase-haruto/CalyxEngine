#include "PlayerDodgeMotion.h"
#include "Engine/PostProcess/Manager/PostEffectManager.h"
#include "PlayerDodgeSystem.h"



#include <numbers>

void PlayerDodgeSpinMotion::Initialize(PlayerDodgeSystem* dodge, WorldTransform* wt) {
	dodge_ = dodge;
	wt_    = wt;

	dodge_->SetOnDodgeStart([this] { OnDodgeStart(); });
	dodge_->SetOnDodgeEnd  ([this] { OnDodgeEnd();   });
	dodge_->SetOnPerfectDodge([this] { OnPerfect();    });
}

void PlayerDodgeSpinMotion::OnDodgeStart() {
	if (!wt_) return;
	baseRot_ = wt_->rotation;
}

void PlayerDodgeSpinMotion::OnDodgeEnd() {
	if (!wt_) return;
	wt_->rotation = baseRot_;
}


void PlayerDodgeSpinMotion::Update(float /*dt*/) {
	if (!dodge_ || !wt_) return;
	if (!dodge_->IsDodging()) return;

	const auto& cfg = dodge_->GetConfig();
	const float total = cfg.startup + cfg.duration + cfg.recovery;
	if (total <= 0.0f) return;

	float t = dodge_->GetStateTime() / total;
	t = std::clamp(t, 0.0f, 1.0f);

	const float rad =
		std::numbers::pi_v<float> * 2.0f * t; // ★ 1回転だけ

	const CxMath::Quaternion spinQ = CxMath::Quaternion::MakeRotateY(rad);
	wt_->rotation = CxMath::Quaternion::Multiply(baseRot_, spinQ);
	wt_->rotationSource = RotationSource::Quaternion;
}

void PlayerDodgeSpinMotion::OnPerfect(){
	if (auto* rb = static_cast<RadialBlurEffect*>(
		PostEffectManager::Get()->GetPass("RadialBlur"))){
		PostEffectManager::Get()->TweenFloat(
			"RadialBlur",
			[rb]{ return rb->GetWidth(); },
			[rb](float v){ rb->SetWidth(v); },
			std::nullopt, 0.05f, 0.10f, Cx::Ease::EaseType::EaseOutExpo, false,
			[rb]{
				PostEffectManager::Get()->TweenFloat(
					"RadialBlur",
					[rb]{ return rb->GetWidth(); },
					[rb](float v){ rb->SetWidth(v); },
					std::nullopt, 0.0f, 0.45f, Cx::Ease::EaseType::EaseOutSine, true
				);
			}
		);
		}

	if (auto* ca = static_cast<ChromaticAberrationEffect*>(
		PostEffectManager::Get()->GetPass("ChromaticAberration"))){
		PostEffectManager::Get()->TweenFloat(
			"ChromaticAberration",
			[ca]{ return ca->GetIntensity(); },
			[ca](float v){ ca->SetIntensity(v); },
			std::nullopt, 0.2f, 0.08f,
			Cx::Ease::EaseType::EaseOutExpo, false,
			[ca]{
				PostEffectManager::Get()->TweenFloat(
					"ChromaticAberration",
					[ca]{ return ca->GetIntensity(); },
					[ca](float v){ ca->SetIntensity(v); },
					std::nullopt, 0.0f, 0.2f,
					Cx::Ease::EaseType::EaseOutSine, true
				);
			}
		);
		}
}