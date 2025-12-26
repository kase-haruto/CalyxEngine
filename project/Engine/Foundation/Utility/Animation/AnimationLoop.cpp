#include "AnimationLoop.h"
//============================================================================
//	include
//============================================================================
// engine
#include <Engine/Foundation/Utility/Converter/EnumConverter.h>
// externals
#include <externals/imgui/imgui.h>
// c++
#include <algorithm>
#include <cmath>

using namespace CalyxUtil;

//============================================================================
//	AnimationLoop classMethods
//============================================================================

AnimationLoop::AnimationLoop()
	: loopCount_(0) // 0 = 無限ループ
	  ,
	  loopType_(AnimationLoopType::PingPong) {}
AnimationLoop::~AnimationLoop() = default;

/////////////////////////////////////////////////////////////////////////////////
//		ループ済みT取得
/////////////////////////////////////////////////////////////////////////////////
float AnimationLoop::LoopedT(float rawT) const {
	// ループなし
	if(loopCount_ == 1) {
		return std::clamp(rawT, 0.0f, 1.0f);
	}

	// loopCount_が0の場合はrawTをそのまま、それ以外はloopCount_を乗算
	const float t = (loopCount_ == 0) ? rawT : rawT * static_cast<float>(loopCount_);

	if(loopType_ == AnimationLoopType::Repeat) {
		return std::fmod(t, 1.0f);
	} else // PingPong
	{
		const float modT = std::fmod(t, 2.0f);
		return (modT <= 1.0f) ? modT : (2.0f - modT);
	}
}
/////////////////////////////////////////////////////////////////////////////////
//		ImGui
/////////////////////////////////////////////////////////////////////////////////
void AnimationLoop::ImGuiLoopParam(bool isSeparate) {

	if(isSeparate) {

		ImGui::SeparatorText("Loop");
	}

	int32_t loopCount = static_cast<int32_t>(loopCount_);
	if(ImGui::DragInt("loopCount", &loopCount, 1, 1)) {

		loopCount_ = static_cast<uint32_t>(loopCount);
	}
}

/////////////////////////////////////////////////////////////////////////////////
//		アニメーション終了到達判定
/////////////////////////////////////////////////////////////////////////////////
bool AnimationLoop::IsReachedEnd(float prevRawT, float currentRawT, float start, float end) const {

	// 正規化
	auto normalize = [start, end](float t) -> float {
		float len = end - start;
		if(len <= 0.0f) {
			return 0.0f;
		}
		return std::clamp((t - start) / len, 0.0f, 1.0f);
	};

	// 正規化済み
	float prevNorm = normalize(prevRawT);
	float curNorm  = normalize(currentRawT);

	// ループなしの場合、単純に1.0到達を判定
	if(loopCount_ <= 1) {
		return (prevNorm < 1.0f && curNorm >= 1.0f);
	}

	// ループありの場合、周回数を考慮して判定
	float prevPhase = prevNorm * static_cast<float>(loopCount_);
	float curPhase	= curNorm * static_cast<float>(loopCount_);

	uint32_t prevIndex = static_cast<uint32_t>(prevPhase); // 前フレームの何周目か
	uint32_t curIndex  = static_cast<uint32_t>(curPhase);  // 今フレームの何周目か

	return prevIndex < curIndex;
}
