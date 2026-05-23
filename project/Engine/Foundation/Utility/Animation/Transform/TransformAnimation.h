#pragma once
//=============================================================================
//	include
//=============================================================================
#include <Engine/Foundation/Clock/StateTimer.h>
#include <Engine/Foundation/Utility/Animation/AnimationLoop.h>
#include <Engine/Objects/Transform/Transform.h>


namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * TransformAnimation
	 * - 位置、回転、スケールのアニメーションを管理するクラス
	 * - 3Dオブジェクトのトランスフォームアニメーションを簡単に実装できるようにするためのクラス
	 * - 0 - 1でトランスフォームを補完してアニメーションさせる
	 *---------------------------------------------------------------------------------------*/
	class TransformAnimation {
	public:
		//========================================================================
		//	private Methods
		//========================================================================
		TransformAnimation()  = default;
		~TransformAnimation() = default;

		/**
		 * \brief 更新
		 * \param dt
		 */
		void Update(float dt);
		/**
		 * \brief デバッグgui
		 */
		void ShowGui();

		bool EaseTypeCombo();

		void Play(float duration);

		// accessor ------------------------------
		// setter
		void SetTarget(BaseTransform* target) { target_ = target; }
		void SetTransformStart(const QuaternionTransform& start) { startTransform_ = start; }
		void SetTransformEnd(const QuaternionTransform& end) { endTransform_ = end; }
		void SetEaseType(CalyxEngine::EaseType type) { easeType_ = type; }

		// getter
		CalyxEngine::EaseType GetEaseType() { return easeType_; }

	private:
		//========================================================================
		//	private Methods
		//========================================================================
		QuaternionTransform LerpTransform(const QuaternionTransform& start, const QuaternionTransform& end, float t) const;

	private:
		//========================================================================
		//	private Variables
		//========================================================================

		QuaternionTransform startTransform_;
		QuaternionTransform endTransform_;

		BaseTransform* target_ = nullptr;

		CalyxEngine::StateTimer	 timer_;
		CalyxEngine::AnimationLoop loop_;

		CalyxEngine::EaseType easeType_ = CalyxEngine::EaseType::Linear;
	};

} // namespace CalyxEngine