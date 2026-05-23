#pragma once
#include "HudMotionDesc.h"
#include "Engine/Foundation/Utility/Converter/EnumConverter.h"
#include "Engine/Foundation/Utility/Animation/SimpleAnimator.h"


namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * HudMotionChannel enum
	 * - HUDモーションチャネル列挙型
	 *---------------------------------------------------------------------------------------*/
	enum class HudMotionChannel : uint32_t {
		None = 0,
		Position = 1 << 0,
		Scale = 1 << 1,
		Alpha = 1 << 2,
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
		HudMotion()  = default;
		~HudMotion() = default;
		/**
		 * \brief 初期化処理
		 * \param flags 有効チャネルフラグ
		 * \example HudMotionChannel::Position | HudMotionChannel::Scale など 追加するchannelをビットORで指定
		 */
		void Initialize(uint32_t flags);

		/**
		 * \brief 入場開始
		 * \param set モーションセット
		 */
		void ApplyMotionSet(const struct HudMotionSet& set);
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		void Update(float dt);
		/**
		 * \brief デバッグ用GUI表示
		 */
		void ShowGui();
		/**
		 * \brief タイムラインGUI表示
		 */
		void ShowTimelineGui() const;
		/**
		 * \brief アニメーション終了判定
		 * \return 終了していれば true
		 */
		bool IsFinished() const;

		void Reset();

		// getters
		const CalyxEngine::Vector2& GetPosition() const { return position_; }
		const CalyxEngine::Vector2& GetScale() const { return scale_; }
		float                     GetRotation() const { return rotation_; }
		float                     GetAlpha() const { return alpha_; }
		bool                      IsChannelEnabled(HudMotionChannel ch) const { return enabledChannels_ & static_cast<uint32_t>(ch); }

	private:
		//===================================================================*/
		//			private methods
		//===================================================================*/
		/**
		 * \brief 値更新
		 * \tparam T
		 * \param out
		 * \param ch
		 */
		template <class T>
		void UpdateValue(T& out,HudMotionChannel ch);
		/**
		 * \brief 指定チャネルのアニメーション終了判定
		 * \tparam T
		 * \param ch
		 * \return 終了していれば true
		 */
		template <class T>
		bool CheckFinished(HudMotionChannel ch) const;
		/**
		 * \brief モーション適用
		 * \tparam T
		 * \param desc モーション記述子
		 * \param channelName チャネル名
		 */
		template <typename T>
		void ApplyMotion(
			const std::optional<HudMotionDesc<T>>& desc,HudMotionChannel motionType);

		/**
		 * \brief チャネル名変換
		 * \param ch
		 * \return
		 */
		static const char* ToChannelName(HudMotionChannel ch);

		// チャンネル制御
		void EnableChannel(HudMotionChannel ch);
		void DisableChannel(HudMotionChannel ch);

	private:
		//===================================================================*/
		//			private members
		//===================================================================*/
		// アニメーター
		CalyxEngine::SimpleAnimator animator_;
		// 有効チャネル
		uint32_t enabledChannels_ = 0;

		// 実値
		CalyxEngine::Vector2 position_;         //< 位置
		CalyxEngine::Vector2 scale_{1.0f,1.0f}; //< スケール
		float              rotation_ = 0.0f;  //< 回転角
		float              alpha_    = 1.0f;  //< 透明度
	};

	//////////////////////////////////////////////////////////////////////////////////
	// 値の更新
	//////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void HudMotion::UpdateValue(T& out,HudMotionChannel ch) { if(enabledChannels_ & (uint32_t)ch) { out = animator_.Get<T>(ToChannelName(ch)); } }

	///////////////////////////////////////////////////////////////////////////////
	// 指定チャネルのアニメーション終了判定
	///////////////////////////////////////////////////////////////////////////////
	template <typename T>
	bool HudMotion::CheckFinished(HudMotionChannel ch) const {
		if(!(enabledChannels_ & (uint32_t)ch)) return true;
		return animator_.GetChannel<T>(ToChannelName(ch))
						.Animation()
						.IsFinished();
	}

	////////////////////////////////////////////////////////////////////////////////
	// モーション適用
	////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	void HudMotion::ApplyMotion(
		const std::optional<HudMotionDesc<T>>& desc,HudMotionChannel motionType) {

		if(!desc) return;
		if(!(enabledChannels_ & static_cast<uint32_t>(motionType))) return;

		auto& ch = animator_.GetChannel<T>(ToChannelName(motionType));
		ch.ResetValue(desc->start);
		ch.Animation().Reset();
		ch.Animation().SetStart(desc->start);
		ch.Animation().SetEnd(desc->end);
		ch.Animation().SetDuration(desc->duration);
		ch.Animation().SetEasing(desc->easing);
		ch.Animation().SetLoopCount(1);
		ch.Animation().Start();
	}

} // namespace CalyxEngine