#include "HudMotion.h"

namespace Calyx2D {

	//////////////////////////////////////////////////////////////////////////////
	//		初期化処理
	//////////////////////////////////////////////////////////////////////////////
	void HudMotion::Initialize(uint32_t flags) {
		enabledChannels_ = flags;

		if(flags & (uint32_t)HudMotionChannel::Position) {
			animator_.Add<CalyxMath::Vector2>("Position").SetLoopCount(1);
			position_ = {};
		}

		if(flags & (uint32_t)HudMotionChannel::Scale) {
			animator_.Add<CalyxMath::Vector2>("Scale").SetLoopCount(1);
			scale_ = {1.0f, 1.0f};
		}

		if(flags & (uint32_t)HudMotionChannel::Alpha) {
			animator_.Add<float>("Alpha").SetLoopCount(1);
			alpha_ = 1.0f;
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	//		入場開始
	//////////////////////////////////////////////////////////////////////////////
	void HudMotion::StartEnter(
		const CalyxMath::Vector2& from,
		const CalyxMath::Vector2& to,
		float					  duration) {

		if(enabledChannels_ & static_cast<uint32_t>(HudMotionChannel::Position)) {
			auto& ch = animator_.GetChannel<CalyxMath::Vector2>("Position");
			ch.ResetValue(from);
			ch.Animation().Reset();
			ch.Animation().SetStart(from);
			ch.Animation().SetEnd(to);
			ch.Animation().SetDuration(duration);
			ch.Animation().Start();
		}

		if(enabledChannels_ & static_cast<uint32_t>(HudMotionChannel::Alpha)) {
			auto& ch = animator_.GetChannel<float>("Alpha");
			ch.ResetValue(0.0f);
			ch.Animation().Reset();
			ch.Animation().SetStart(0.0f);
			ch.Animation().SetEnd(1.0f);
			ch.Animation().SetDuration(duration * 0.8f);
			ch.Animation().Start();
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	//		退場開始
	////////////////////////////////////////////////////////////////////////////////
	void HudMotion::StartExit(
		const CalyxMath::Vector2& from,
		const CalyxMath::Vector2& to,
		float					  duration) {

		if(enabledChannels_ & static_cast<uint32_t>(HudMotionChannel::Position)) {
			auto& ch = animator_.GetChannel<CalyxMath::Vector2>("Position");
			ch.ResetValue(from);
			ch.Animation().Reset();
			ch.Animation().SetStart(from);
			ch.Animation().SetEnd(to);
			ch.Animation().SetDuration(duration);
			ch.Animation().Start();
		}

		if(enabledChannels_ & static_cast<uint32_t>(HudMotionChannel::Alpha)) {
			auto& ch = animator_.GetChannel<float>("Alpha");
			ch.ResetValue(1.0f);
			ch.Animation().Reset();
			ch.Animation().SetStart(1.0f);
			ch.Animation().SetEnd(0.0f);
			ch.Animation().SetDuration(duration * 0.6f);
			ch.Animation().Start();
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	//		終了判定
	///////////////////////////////////////////////////////////////////////////////
	bool HudMotion::IsFinished() const {

		bool finished = true;

		if(enabledChannels_ & static_cast<uint32_t>(HudMotionChannel::Position))
			finished &= animator_.GetChannel<CalyxMath::Vector2>("Position")
							.Animation()
							.IsFinished();

		if(enabledChannels_ & static_cast<uint32_t>(HudMotionChannel::Alpha))
			finished &= animator_.GetChannel<float>("Alpha")
							.Animation()
							.IsFinished();

		return finished;
	}

} // namespace Calyx2D
