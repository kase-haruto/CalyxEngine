#include "HudMotion.h"

#include "Game/Battle/Shooting/Pattern/ShootPatternDetails.h"
#include "HudMotionSet.h"

namespace Calyx2D {

	//////////////////////////////////////////////////////////////////////////////
	//		初期化処理
	//////////////////////////////////////////////////////////////////////////////
	void HudMotion::Initialize(uint32_t flags) {
		enabledChannels_ = flags;
		// アニメーションチャンネル追加

		// 移動チャンネル
		if(flags & static_cast<uint32_t>(HudMotionChannel::Position)) { animator_.Add<CalyxMath::Vector2>(ToChannelName(HudMotionChannel::Position)).SetLoopCount(1); }

		// スケールチャンネル
		if(flags & static_cast<uint32_t>(HudMotionChannel::Scale)) { animator_.Add<CalyxMath::Vector2>(ToChannelName(HudMotionChannel::Scale)).SetLoopCount(1); }

		// 透明度チャンネル
		if(flags & static_cast<uint32_t>(HudMotionChannel::Alpha)) { animator_.Add<float>(ToChannelName(HudMotionChannel::Alpha)).SetLoopCount(1); }

		// 回転チャンネル
		if(flags & static_cast<uint32_t>(HudMotionChannel::Rotation)) { animator_.Add<float>(ToChannelName(HudMotionChannel::Rotation)).SetLoopCount(1); }
	}

	//////////////////////////////////////////////////////////////////////////////
	//		入場開始
	//////////////////////////////////////////////////////////////////////////////
	void HudMotion::ApplyMotionSet(const HudMotionSet& set) {
		ApplyMotion(set.position,HudMotionChannel::Position);
		ApplyMotion(set.scale,HudMotionChannel::Scale);
		ApplyMotion(set.alpha,HudMotionChannel::Alpha);
		ApplyMotion(set.rotation,HudMotionChannel::Rotation);
	}

	///////////////////////////////////////////////////////////////////////////////
	//		更新処理
	///////////////////////////////////////////////////////////////////////////////
	void HudMotion::Update(float dt) {
		animator_.Update(dt);

		if(IsChannelEnabled(HudMotionChannel::Position))
			UpdateValue(position_,HudMotionChannel::Position);

		if(IsChannelEnabled(HudMotionChannel::Scale))
			UpdateValue(scale_,HudMotionChannel::Scale);

		if(IsChannelEnabled(HudMotionChannel::Rotation))
			UpdateValue(rotation_,HudMotionChannel::Rotation);

		if(IsChannelEnabled(HudMotionChannel::Alpha))
			UpdateValue(alpha_,HudMotionChannel::Alpha);
	}

	///////////////////////////////////////////////////////////////////////////////
	//		終了判定
	///////////////////////////////////////////////////////////////////////////////
	bool HudMotion::IsFinished() const { return CheckFinished<CalyxMath::Vector2>(HudMotionChannel::Position) && CheckFinished<CalyxMath::Vector2>(HudMotionChannel::Scale) && CheckFinished<float>(HudMotionChannel::Rotation) && CheckFinished<float>(HudMotionChannel::Alpha); }

	///////////////////////////////////////////////////////////////////////////////
	//		チャンネル名取得
	///////////////////////////////////////////////////////////////////////////////
	const char* HudMotion::ToChannelName(HudMotionChannel ch) {
		switch(ch) {
		case HudMotionChannel::Position:
			return "Position";
		case HudMotionChannel::Scale:
			return "Scale";
		case HudMotionChannel::Alpha:
			return "Alpha";
		case HudMotionChannel::Rotation:
			return "Rotation";
		default:
			return "";
		}
	}

} // namespace Calyx2D