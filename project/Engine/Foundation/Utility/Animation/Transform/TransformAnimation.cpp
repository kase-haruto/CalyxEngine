#include "TransformAnimation.h"

#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>
#include "Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h"

///////////////////////////////////////////////////////////////////////////////////////////
//		更新
///////////////////////////////////////////////////////////////////////////////////////////
void CalyxEngine::TransformAnimation::Update(float dt) {
	if(!target_) return;

	timer_.Update(dt);

	float rawT = timer_.GetEasedT();

	float t = loop_.LoopedT(rawT);

	float eased = CalyxEase::ApplyEase(easeType_,t);

	BaseTransform result = LerpTransform(startTransform_,endTransform_,eased);

	*target_ = result;
}

///////////////////////////////////////////////////////////////////////////////////////////
//		デバッグよう
///////////////////////////////////////////////////////////////////////////////////////////
void CalyxEngine::TransformAnimation::ShowGui() {
	// イージングタイプ設定
	CalyxUtil::EnumConverter<CalyxEase::EaseType>::Combo("easeType", easeType_);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		更新
///////////////////////////////////////////////////////////////////////////////////////////
void CalyxEngine::TransformAnimation::Play(float duration) {
	timer_.Reset();
	timer_.SetTarget(duration);
}

BaseTransform CalyxEngine::TransformAnimation::LerpTransform(const BaseTransform& start,
															 const BaseTransform& end,
															 float                t) const {
	BaseTransform r;

	r.translation = CalyxMath::Vector3::Lerp(start.translation,end.translation,t);
	r.scale       = CalyxMath::Vector3::Lerp(start.scale,end.scale,t);

	r.rotation = CalyxMath::Quaternion::Slerp(
		start.rotation,
		end.rotation,
		t
		);

	return r;
}