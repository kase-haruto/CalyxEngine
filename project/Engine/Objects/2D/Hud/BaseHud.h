#pragma once
/* ==========================================================================
 *	include space
 * ========================================================================*/
#include "HudMotion.h"
#include "HudMotionSet.h"

#include "../Object2d/SpriteObject2d.h"

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * HUD設定構造体
	 * - HUDの登場・退場の設定をまとめた構造体
	 *---------------------------------------------------------------------------------------*/
	struct HudConfig {
		std::string texturePath; //< テクスチャパス

		HudMotionSet enterMotion; //< 登場モーション
		HudMotionSet exitMotion;  //< 退場モーション
	};

	/*-----------------------------------------------------------------------------------------
	 * HUD基底クラス
	 * - HUDのフェーズ管理と描画を担当
	 * - アニメーション処理は HudMotion に委譲する
	 *---------------------------------------------------------------------------------------*/
	class BaseHud {
		//===================================================================*/
		//                    structs / enums
		//===================================================================*/
		enum class HudPhase {
			Enter, //< 登場
			Stay,  //< 滞在
			Exit,  //< 退場
			End,   //< 終了
		};

	public:
		//===================================================================*/
		//                    public methods
		//===================================================================*/
		/** \brief コンストラクタ / デストラクタ */
		BaseHud();
		virtual ~BaseHud();

		/**
		 * \brief 初期化処理
		 * \param config HUD設定
		 * \param moveFlags 動作チャネルフラグ (デフォルトですべて有効)
		 * \note 派生クラスは呼び出す前にコンフィグを構築して設定しておくこと
		 */
		void Initialize(uint32_t moveFlags =
			static_cast<uint32_t>(HudMotionChannel::Position) |
			static_cast<uint32_t>(HudMotionChannel::Scale) |
			static_cast<uint32_t>(HudMotionChannel::Alpha) |
			static_cast<uint32_t>(HudMotionChannel::Rotation));

		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		virtual void Update(float dt);
		/**
		 * \brief デバッグ用GUI表示
		 */
		void ShowGui();
		/**
		 * \brief 描画処理
		 * \param renderer スプライトレンダラー
		 */
		virtual void Draw(SpriteRenderer* renderer) const;

		/**
		 * \brief HUDの登場を開始
		 */
		void StartEnter();

		/**
		 * \brief HUDの退場を開始
		 */
		void StartExit();

		//===================================================================*/
		//                    accessor
		//===================================================================*/
		bool             IsFinished() const { return phase_ == HudPhase::End; }
		const HudMotion& GetMotion() const { return motion_; }
		const HudConfig& GetConfig() const { return config_; }
		void SetConfig(const HudConfig& config) { config_ = config; }

	private:
		//===================================================================*/
		//                    private methods
		//===================================================================*/
		/**
		 * \brief モーション値適用
		 */
		void ApplyMotionValue() const;

	protected:
		//===================================================================*/
		//                    protected methods
		//===================================================================*/
		/**
		 * \brief 滞在フェーズ更新
		 */
		virtual void StayUpdate(float dt);

		/**
		 * \brief スプライト取得
		 */
		SpriteObject2d& Sprite() const { return *spriteObj_; }

		/**
		 * \brief 登場完了時処理
		 */
		virtual void OnEnterFinished() {}

		/**
		 * \brief 退場完了時処理
		 */
		virtual void OnExitFinished() {}

		/**
		 * \brief トップに表示するGUI
		 */
		virtual void TopGui() {}
		/**
		 * \brief 派生先Gui
		 */
		virtual void DerivedGui() {}

	protected:
		//===================================================================*/
		//                    protected members
		//===================================================================*/
		HudPhase  phase_ = HudPhase::Enter; //< フェーズ
		HudConfig config_;                  //< HUD設定

		HudMotion motion_; //< HUDモーション

	private:
		//===================================================================*/
		//                    private members
		//===================================================================*/
		std::unique_ptr<SpriteObject2d> spriteObj_ = nullptr; //< スプライト
	};

} // namespace CalyxEngine