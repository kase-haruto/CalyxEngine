#include "TransformAnimation.h"

#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h"
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>

///////////////////////////////////////////////////////////////////////////////////////////
//		更新
///////////////////////////////////////////////////////////////////////////////////////////
void CalyxEngine::TransformAnimation::Update(float dt) {
	if(!target_) return;

	timer_.Update(dt);

	float rawT = timer_.GetEasedT();

	float t = loop_.LoopedT(rawT);

	float eased = CalyxEase::ApplyEase(easeType_, t);

	QuaternionTransform result = LerpTransform(startTransform_, endTransform_, eased);

	// データをコピー (BaseTransformそのものを代入するとDxBufferのリソースが壊れるため)
	target_->scale		 = result.scale;
	target_->rotation	 = result.rotate;
	target_->translation = result.translate;
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

QuaternionTransform CalyxEngine::TransformAnimation::LerpTransform(const QuaternionTransform& start,
																   const QuaternionTransform& end,
																   float					  t) const {
	QuaternionTransform r;

	r.translate = CalyxMath::Vector3::Lerp(start.translate, end.translate, t);
	r.scale		= CalyxMath::Vector3::Lerp(start.scale, end.scale, t);

	r.rotate = CalyxMath::Quaternion::Slerp(
		start.rotate,
		end.rotate,
		t);

	return r;
}