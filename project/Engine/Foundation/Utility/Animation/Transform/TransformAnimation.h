#pragma once
//=============================================================================
//	include
//=============================================================================
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Foundation/Utility/Animation/AnimationLoop.h>
#include <Engine/Foundation/Clock/StateTimer.h>

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

		void Play(float duration);

		// accessor ------------------------------
		// setter
		void SetTarget(BaseTransform* target) { target_ = target; }
		void SetTransformStart(const BaseTransform& start) { startTransform_ = start; }
		void SetTransformEnd(const BaseTransform& end) { endTransform_ = end; }
		void SetEaseType(CalyxEase::EaseType type) { easeType_ = type; }

	private:
		//========================================================================
		//	private Methods
		//========================================================================
		BaseTransform LerpTransform(const BaseTransform& start,const BaseTransform& end,float t) const;

	private:
		//========================================================================
		//	private Methods
		//========================================================================

		BaseTransform startTransform_;
		BaseTransform endTransform_;

		BaseTransform* target_ = nullptr;

		CalyxUtil::StateTimer    timer_;
		CalyxUtil::AnimationLoop loop_;

		CalyxEase::EaseType easeType_ = CalyxEase::EaseType::Linear;
	};

}