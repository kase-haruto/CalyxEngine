#pragma once

//============================================================================
//	include
//============================================================================

// engine
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

// c++
#include <optional>
#include <string>

//============================================================================
//	StateTimer
//	イージング対応の状態遷移用タイマー
//============================================================================
namespace CalyxEngine {

	class StateTimer {
	public:
		//========================================================================
		//	constructor
		//========================================================================
		StateTimer() = default;
		explicit StateTimer(float target, CalyxEngine::EaseType easing = CalyxEngine::EaseType::Linear);

		//========================================================================
		//	update
		//========================================================================
		void Update(float						deltaTime,
					const std::optional<float>& newTarget = std::nullopt);

		//========================================================================
		//	control
		//========================================================================
		void Reset();
		void SetTarget(float target);
		void SetEasing(CalyxEngine::EaseType type);

		//========================================================================
		//	query
		//========================================================================
		bool IsReached() const;
		bool IsRunning() const;

		float GetCurrent() const { return current_; }
		float GetTarget() const { return target_; }
		float GetT() const { return t_; }
		float GetEasedT() const { return easedT_; }

		//========================================================================
		//	editor
		//========================================================================
		void ImGui(const std::string& label, bool separate = true);

	private:
		//========================================================================
		//	internal
		//========================================================================
		void UpdateT();
		void UpdateEasing();

	public:
		//========================================================================
		//	variables
		//========================================================================
		float current_ = 0.0f; // 現在時間
		float target_  = 1.0f; // 到達時間

		float t_	  = 0.0f; // 正規化T [0-1]
		float easedT_ = 0.0f; // イージング後T

		 CalyxEngine::EaseType easingType_ =  CalyxEngine::EaseType::Linear;
	};

} // namespace CalyxEngine
