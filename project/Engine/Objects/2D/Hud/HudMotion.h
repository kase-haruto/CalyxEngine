#pragma once
#include "Engine/Foundation/Math/Vector2.h"
#include <Engine/Foundation/Utility/Animation/SimpleAnimator.h>

namespace Calyx2D {

	/*-----------------------------------------------------------------------------------------
	 * HudMotionChannel enum
	 * - HUDモーションチャネル列挙型
	 *---------------------------------------------------------------------------------------*/
	enum class HudMotionChannel : uint32_t {
		None     = 0,
		Position = 1 << 0,
		Scale    = 1 << 1,
		Alpha    = 1 << 2,
		Rotation = 1 << 3,
	};
	
	/*-----------------------------------------------------------------------------------------
	 * HudMotion class
	 * - HUDモーションクラス
	 * - フェーズごとのHUDアニメーションを管理
	 *---------------------------------------------------------------------------------------*/
	class HudMotion {
	public:
		//===================================================================*/
		//			public methods
		//===================================================================*/
		/** \brief コンストラクタ / デストラクタ*/
		HudMotion()	 = default;
		~HudMotion() = default;
		/**
		 * \brief 初期化処理
		 */
		void Initialize(uint32_t flags);
		/**
		 * \brief 入場アニメーション開始
		 * \param from 開始位置
		 * \param to 終了位置
		 * \param duration アニメーション時間
		 */
		void StartEnter(
			const CalyxMath::Vector2& from,
			const CalyxMath::Vector2& to,
			float					  duration) ;
		/**
		 * \brief 退場アニメーション開始
		 * \param from 開始位置
		 * \param to 終了位置
		 * \param duration アニメーション時間
		 */
		void StartExit(
			const CalyxMath::Vector2& from,
			const CalyxMath::Vector2& to,
			float					  duration);
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		void Update(float dt);
		/**
		 * \brief 位置取得
		 * \return 位置
		 */
		const CalyxMath::Vector2& GetPosition() const { return position_; }
		/**
		 * \brief サイズ取得
		 * \return サイズ
		 */
		float					  GetAlpha() const { return alpha_; }
		/**
		 * \brief アニメーション終了判定
		 * \return 終了していれば true
		 */
		bool IsFinished() const;

	private:
		//===================================================================*/
		//			private members
		//===================================================================*/
		// アニメーター
		CalyxUtil::SimpleAnimator animator_;
		// 有効チャネル
		uint32_t enabledChannels_ = 0;

		// 実値
		CalyxMath::Vector2 position_;		 //< 位置
		CalyxMath::Vector2 scale_;			 //< スケール
		float			   rotation_ = 0.0f; //< 回転角
		float			   alpha_	 = 1.0f; //< 透明度
	};

} // namespace Calyx2D
