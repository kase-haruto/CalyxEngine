#pragma once

/* ========================================================================
/* 	include space
/* ===================================================================== */
#include <cstdint>

namespace CalyxUtil {

	/*------------------------------------------------------------------------*/
	/* アニメーションループ制御クラス
	/* - ループ回数の管理
	/* - 0.0~1.0の範囲でループさせた値を取得
	/* - ImGuiでパラメータ編集
	/*------------------------------------------------------------------------*/
	class AnimationLoop {
	public:
		/* ========================================================================
		/* 	enums
		/* ===================================================================== */
		enum class AnimationLoopType {
			Repeat,	 //< 連続ループ
			PingPong //< 往復ループ
		};

	public:
		//========================================================================
		//	public Methods
		//========================================================================
		/** \brief コンストラクタ / デストラクタ */
		AnimationLoop();
		~AnimationLoop();
		/**
		 * \brief 0.0~1.0の範囲でループさせた値を取得
		 * \param rawT tの生値
		 */
		float LoopedT(float rawT) const;
		/**
		 * \brief ImGuiでパラメータ編集
		 * \param isSeparate true:区切り線を入れる、false:入れない
		 */
		void ImGuiLoopParam(bool isSeparate = true);

		//--------- accessor -----------------------------------------------------
		/**
		 * \brief ループカウントを設定
		 * \param count ループ回数
		 */
		void SetLoopCount(uint32_t count) { loopCount_ = count; }
		/**
		 * \brief 終わったかどうか
		 * \param prevRawT		// 前回の生値
		 * \param currentRawT	// 今回の生値
		 * \param start			// 開始値
		 * \param end			// 終了値
		 * \return 終了に到達したかどうか
		 */
		bool IsReachedEnd(float prevRawT, float currentRawT,
						  float start = 0.0f, float end = 1.0f) const;
		/**
		 * \brief ループ回数を取得
		 * \return ループ回数
		 */
		uint32_t GetLoopCount() const { return loopCount_; }
		
		void SetLoopTypePingPong() { loopType_ = AnimationLoopType::PingPong; }
		void SetLoopTypeRepeat()  { loopType_ = AnimationLoopType::Repeat;  }
	private:
		//========================================================================
		//	private Methods
		//========================================================================
		// ループ制御
		uint32_t		  loopCount_; // 回数(0:無限)
		AnimationLoopType loopType_;  // 種類
	};

} // namespace CalyxUtil