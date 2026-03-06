#pragma once

//============================================================================
//	include
//============================================================================
#include <Engine/Foundation/Clock/StateTimer.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Utility/Animation/AnimationLoop.h>

// c++
#include <optional>

// imgui
#include "Engine/Foundation/Math/Vector2.h"
#include "Engine/Foundation/Math/Vector3.h"
#include "Engine/Foundation/Math/Vector4.h"
#include "imgui/imgui.h"

//============================================================================
//	SimpleAnimation enum class
//============================================================================

// 補間の仕方
enum class SimpleAnimationType {

	None,  // start -> end
	Return // end -> start
};

//============================================================================
//	SimpleAnimation class
//============================================================================
namespace CalyxUtil {

	/*-----------------------------------------------------------------------------------------
	 * SimpleAnimation
	 * - startからendまで補間するシンプルなアニメーションクラス
	 * - イージングやループに対応
	 *---------------------------------------------------------------------------------------*/
	template <typename T>
	class SimpleAnimation {
	public:
		//========================================================================
		//	public Methods
		//========================================================================
		/** \brief コンストラクタ・デストラクタ */
		SimpleAnimation()  = default;
		~SimpleAnimation() = default;

		/**
		 * \brief ImGui表示
		 * \param label
		 * \param isLoop
		 */
		void ImGui(const std::string& label, bool isLoop = true);
		/**
		 * \brief 補間値取得
		 * \param value 補間値の参照
		 * \param dt デルタタイム
		 */
		void LerpValue(T& value, float dt);
		/**
		 * \brief 開始
		 */
		void Start();
		/**
		 * \brief リセット
		 * \param isStop 停止するかどうか
		 */
		void Reset(bool isStop = true);
		/**
		 * \brief 停止
		 */
		void Stop();

		//--------- accessor -----------------------------------------------------

		void SetStart(const T& start) { move_.start = start; }
		void SetEnd(const T& end) { move_.end = end; }
		void SetAnimationType(SimpleAnimationType type) { type_ = type; }
		void SetDragValue(float value);
		void SetDragValue(int value);
		void SetLoopType(AnimationLoop::AnimationLoopType type);
		void SetLoopCount(int count);
		void SetEasing(CalyxEase::EaseType ease) { timer_.easingType_ = ease; }
		void SetReturnEasing(CalyxEase::EaseType ease) { returnTimer_.easingType_ = ease; }
		void SetDuration(float seconds) { timer_.SetTarget(seconds); }

		bool IsStart() const { return isRunning_; }
		bool IsFinished() const { return isFinished_; }

		float GetDuration() const { return timer_.target_; }
		float				 GetProgress() const;
		const T&			 GetStart() const { return move_.start; }
		const T&			 GetEnd() const { return move_.end; }
		const AnimationLoop& GetLoop() const { return loop_; }

	private:
		//========================================================================
		//	private Methods
		//========================================================================

		//--------- structure ----------------------------------------------------

		struct Move {
			T start; // 開始値
			T end;	 // 終了値
		};

		//--------- variables ----------------------------------------------------

		SimpleAnimationType type_ = SimpleAnimationType::None;
		bool				isRunning_		= false;
		bool				isFinished_		= false;
		bool				useReturnTimer_ = false;

		StateTimer	  timer_;
		StateTimer	  returnTimer_;
		float		  rawT_ = 0.0f;
		AnimationLoop loop_;
		Move		  move_;

		// Drag値
		int	  dragValueInt	 = 1;
		float dragValueFloat = 0.01f;

		// imguiのサイズ
		const float itemSize_ = 224.0f;
	};

	//============================================================================
	//	SimpleAnimation templateMethods
	//============================================================================

	template <typename T>
	inline void SimpleAnimation<T>::LerpValue(T& value, float dt) {

		// 実行中でなければ固定
		if(!isRunning_) {
			return;
		}

		// イージング用タイマー更新
		if(type_ == SimpleAnimationType::Return && useReturnTimer_) {
			returnTimer_.Update(dt);
		} else {
			timer_.Update(dt);
		}

		rawT_ += dt / (std::max)(0.0001f, timer_.target_);

		// ループ・PingPong適用
		float loopedT = loop_.LoopedT(rawT_);

		// イージング
		float easedT = CalyxEase::ApplyEase(timer_.easingType_, loopedT);

		// 補間方向
		T from = move_.start;
		T to   = move_.end;
		if(type_ == SimpleAnimationType::Return) {
			std::swap(from, to);
		}

		// 補間
		if constexpr(std::is_same_v<T, float>) {
			value = CalyxMath::Lerp(from, to, easedT);
		} else if constexpr(std::is_same_v<T, CalyxMath::Vector2>) {
			value = CalyxMath::Vector2::Lerp(from, to, easedT);
		} else if constexpr(std::is_same_v<T, CalyxMath::Vector3>) {
			value = CalyxMath::Vector3::Lerp(from, to, easedT);
		} else if constexpr(std::is_same_v<T, CalyxMath::Vector4>) {
			value = CalyxMath::Vector4::Lerp(from, to, easedT);
		}else if constexpr(std::is_same_v<T, CalyxMath::Quaternion>) {
			value = CalyxMath::Quaternion::Slerp(from, to, easedT);
		}

		// ================================
		// 終了判定（無限ループは止めない）
		// ================================
		if(loop_.GetLoopCount() != 0) {
			if(rawT_ >= static_cast<float>(loop_.GetLoopCount())) {
				isFinished_ = true;
				isRunning_	= false;
			}
		}
	}

	template <typename T>
	inline void SimpleAnimation<T>::Start() {
		isRunning_	= true;
		isFinished_ = false;
		rawT_		= 0.0f;
		timer_.Reset();
		returnTimer_.Reset();
	}

	template <typename T>
	inline void SimpleAnimation<T>::Reset(bool isStop) {
		isRunning_	= !isStop;
		isFinished_ = false;
		rawT_		= 0.0f;
		timer_.Reset();
		returnTimer_.Reset();
	}

	template <typename T>
	inline void SimpleAnimation<T>::Stop() {

		isRunning_ = false;
	}

	template <typename T>
	inline void SimpleAnimation<T>::ImGui(const std::string& label, bool isLoop) {

		ImGui::PushItemWidth(itemSize_);
		ImGui::PushID(label.c_str());

		ImGui::SeparatorText(label.c_str());

		ImGuiTreeNodeFlags windowFlag = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf;
		if(ImGui::CollapsingHeader("AnimValue", windowFlag)) {

			if constexpr(std::is_same_v<T, float>) {

				ImGui::DragFloat("start", &move_.start, dragValueFloat);
				ImGui::DragFloat("end", &move_.end, dragValueFloat);
			} else if constexpr(std::is_same_v<T, int>) {

				ImGui::DragInt("start", &move_.start, static_cast<float>(dragValueInt));
				ImGui::DragInt("end", &move_.end, static_cast<float>(dragValueInt));
			} else if constexpr(std::is_same_v<T, CalyxMath::Vector2>) {

				ImGui::DragFloat2("start", &move_.start.x, dragValueFloat);
				ImGui::DragFloat2("end", &move_.end.x, dragValueFloat);
			} else if constexpr(std::is_same_v<T, CalyxMath::Vector3>) {

				ImGui::DragFloat3("start", &move_.start.x, dragValueFloat);
				ImGui::DragFloat3("end", &move_.end.x, dragValueFloat);
			} else if constexpr(std::is_same_v<T, CalyxMath::Vector4>) {

				ImGui::ColorEdit4("start", &move_.start.x);
				ImGui::ColorEdit4("end", &move_.end.x);
			} else if constexpr(std::is_same_v<T, CalyxMath::Quaternion>) {
				ImGui::DragFloat4("start", &move_.start.x, dragValueFloat);
				ImGui::DragFloat4("end", &move_.end.x, dragValueFloat);
			}
		}
		if(ImGui::CollapsingHeader("Timer", windowFlag)) {

			timer_.ImGui("Time", false);

			ImGui::Checkbox("useReturnTimer", &useReturnTimer_);
			if(useReturnTimer_) {

				returnTimer_.ImGui("ReturnTimer", true);
			}
		}
		if(isLoop) {
			if(ImGui::CollapsingHeader("Loop", windowFlag)) {

				loop_.ImGuiLoopParam(false);
			}
		}

		ImGui::PopID();
		ImGui::PopItemWidth();
	}

	template <typename T>
	inline void SimpleAnimation<T>::SetDragValue(float value) {

		dragValueFloat = value;
	}

	template <typename T>
	inline void SimpleAnimation<T>::SetDragValue(int value) {

		dragValueInt = value;
	}
	template <typename T>
	void SimpleAnimation<T>::SetLoopType(AnimationLoop::AnimationLoopType type) {
		// ループタイプ設定
		if(AnimationLoop::AnimationLoopType::PingPong == type) {
			// ピングポン設定
			loop_.SetLoopTypePingPong();
		} else {
			// リピート設定
			loop_.SetLoopTypeRepeat();
		}
	}
	template <typename T>
	void SimpleAnimation<T>::SetLoopCount(int count) {
		loop_.SetLoopCount(count);
	}

	template <typename T>
	inline float SimpleAnimation<T>::GetProgress() const {

		if(type_ == SimpleAnimationType::None) {

			return timer_.t_;
		} else if(type_ == SimpleAnimationType::Return) {
			if(useReturnTimer_) {

				return returnTimer_.t_;
			}
			return timer_.t_;
		}
		return 0.0f;
	}

}; // namespace CalyxUtil