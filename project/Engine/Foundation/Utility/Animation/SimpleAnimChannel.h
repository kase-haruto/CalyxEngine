#pragma once

#include "SimpleAnimation.h"

namespace CalyxEngine {

	/*----------------------------------------------------------------------------*/
	/* SimpleAnimChannel class
	/* - SimpleAnimationのチャンネル
	/* - 現在の値を保持し、アニメーションの更新を行う
	/*----------------------------------------------------------------------------*/
	template <typename T>
	class SimpleAnimChannel {
	public:
		SimpleAnimation<T>&       Animation() { return animation_; }
		const SimpleAnimation<T>& Animation() const { return animation_; }
		void Reset();
		/**
		 * \brief アニメーションの更新
		 * \param dt
		 */
		void Update(float dt) { animation_.LerpValue(current_,dt); }
		/**
		 * \brief 現在の値を取得
		 * \return 
		 */
		const T& GetValue() const { return current_; }
		/**
		 * \brief 現在の値をリセット
		 * \param v 
		 */
		void                 ResetValue(const T& v = T{}) { current_ = v; }
		const AnimationLoop& GetLoop() const { return animation_.GetLoop(); }
		void                 ShowGui(const std::string& label,bool isLoop = true) { animation_.ImGui(label,isLoop); }
		void                 SetLoopCount(int32_t loopCount) { animation_.SetLoopCount(loopCount); }
		float                GetProgress() const { return animation_.GetProgress(); }
		float                GetDuration() const { return animation_.GetDuration(); }
		bool                 IsRunning() const { return animation_.IsStart(); }
		bool                 IsFinished() const { return animation_.IsFinished(); }

	private:
		SimpleAnimation<T> animation_;
		T                  current_{};
	};

	template <typename T>
	void SimpleAnimChannel<T>::Reset() {
		animation_.Reset();
	}

} // namespace CalyxEngine