#include "HudMotion.h"

#include "HudMotionSet.h"

namespace CalyxEngine {

	//////////////////////////////////////////////////////////////////////////////
	//		初期化処理
	//////////////////////////////////////////////////////////////////////////////
	void HudMotion::Initialize(uint32_t flags) {
		enabledChannels_ = flags;
		// アニメーションチャンネル追加

		// 移動チャンネル
		if(flags & static_cast<uint32_t>(HudMotionChannel::Position)) { animator_.Add<CalyxEngine::Vector2>(ToChannelName(HudMotionChannel::Position)).SetLoopCount(1); }

		// スケールチャンネル
		if(flags & static_cast<uint32_t>(HudMotionChannel::Scale)) { animator_.Add<CalyxEngine::Vector2>(ToChannelName(HudMotionChannel::Scale)).SetLoopCount(1); }

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

	////////////////////////////////////////////////////////////////////////////////
	//		GUI表示
	////////////////////////////////////////////////////////////////////////////////
	void HudMotion::ShowGui() {
		ImGui::SeparatorText("HudMotion Channels");

		auto drawChannel = [&](HudMotionChannel ch, const char* label) {
			bool enabled = IsChannelEnabled(ch);
			if(ImGui::Checkbox(label, &enabled)) {
				if(enabled) {
					EnableChannel(ch);
				} else {
					DisableChannel(ch);
				}
			}
		};

		drawChannel(HudMotionChannel::Position, "Position");
		drawChannel(HudMotionChannel::Scale, "Scale");
		drawChannel(HudMotionChannel::Rotation, "Rotation");
		drawChannel(HudMotionChannel::Alpha, "Alpha");

		ImGui::Spacing();

		ImGui::SeparatorText("Runtime Animator");
		animator_.ShowGui(true);
	}

	////////////////////////////////////////////////////////////////////////////////
	//		タイムラインGUI表示
	////////////////////////////////////////////////////////////////////////////////
	void HudMotion::ShowTimelineGui() const {
		ImGui::SeparatorText("Timeline");

		const float width  = ImGui::GetContentRegionAvail().x;
		const float height = 18.0f;

		auto drawTrack = [&](const char* label,
					 float duration,
					 float progress) {

			ImGui::Text("%s (%.2fs)", label, duration);

			ImVec2 p = ImGui::GetCursorScreenPos();
			ImDrawList* dl = ImGui::GetWindowDrawList();

			// 背景
			dl->AddRectFilled(
				p,
				{ p.x + width, p.y + height },
				IM_COL32(60, 60, 60, 255));

			// 再生位置
			float x = p.x + width * progress;
			dl->AddLine(
				{ x, p.y },
				{ x, p.y + height },
				IM_COL32(255, 220, 0, 255),
				2.0f);

			ImGui::Dummy({ width, height });
		};

		if(IsChannelEnabled(HudMotionChannel::Position)) {
			const auto& ch = animator_.GetChannel<CalyxEngine::Vector2>("Position");
			drawTrack("Position",
				ch.GetDuration(),
				ch.GetProgress());
		}

		if(IsChannelEnabled(HudMotionChannel::Scale)) {
			const auto& ch = animator_.GetChannel<CalyxEngine::Vector2>("Scale");
			drawTrack("Scale",
				ch.GetDuration(),
				ch.GetProgress());
		}

		if(IsChannelEnabled(HudMotionChannel::Rotation)) {
			const auto& ch = animator_.GetChannel<float>("Rotation");
			drawTrack("Rotation",
				ch.GetDuration(),
				ch.GetProgress());
		}

		if(IsChannelEnabled(HudMotionChannel::Alpha)) {
			const auto& ch = animator_.GetChannel<float>("Alpha");
			drawTrack("Alpha",
				ch.GetDuration(),
				ch.GetProgress());
		}
	}

	void HudMotion::EnableChannel(HudMotionChannel ch) {
		enabledChannels_ |= static_cast<uint32_t>(ch);
	}

	void HudMotion::DisableChannel(HudMotionChannel ch) {
		enabledChannels_ &= ~static_cast<uint32_t>(ch);
	}

	///////////////////////////////////////////////////////////////////////////////
	//		終了判定
	///////////////////////////////////////////////////////////////////////////////
	bool HudMotion::IsFinished() const { return CheckFinished<CalyxEngine::Vector2>(HudMotionChannel::Position) && CheckFinished<CalyxEngine::Vector2>(HudMotionChannel::Scale) && CheckFinished<float>(HudMotionChannel::Rotation) && CheckFinished<float>(HudMotionChannel::Alpha); }

	////////////////////////////////////////////////////////////////////////////////
	/// 	リセット
	////////////////////////////////////////////////////////////////////////////////
	void HudMotion::Reset() {
		// アニメーターリセット
		animator_.Reset();
	}

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

} // namespace CalyxEngine