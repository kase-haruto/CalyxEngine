#pragma once
#include "HudMotion.h"
#include "HudMotionSet.h"
#include "../Object2d/SpriteObject2d.h"

namespace CalyxEngine {
	class BaseHud;

	/*-----------------------------------------------------------------------------------------
	 * IHudState
	 * - HUD状態の開始、更新、終了処理を定義するインターフェース
	 * - HUDの描画リソースとモーションの所有権は管理しない
	 *---------------------------------------------------------------------------------------*/
	class IHudState {
	public:
		/** \brief HUD状態の基底デストラクタ */
		virtual ~IHudState() = default;
		/** \brief 状態開始処理を実行する \param hud 状態を保持するHUD */
		virtual void Enter(BaseHud& hud) const = 0;
		/** \brief 状態中の処理を更新する \param hud 状態を保持するHUD \param dt 前フレームからの経過時間（秒） */
		virtual void Update(BaseHud& hud, float dt) const = 0;
		/** \brief 状態終了処理を実行する \param hud 状態を保持するHUD */
		virtual void Exit(BaseHud& hud) const = 0;
	};

	class HudEnterState;
	class HudStayState;
	class HudExitState;
	class HudEndState;

	/*-----------------------------------------------------------------------------------------
	 * HudConfig
	 * - HUDの表示と登場・退場モーション設定を保持するデータ構造
	 * - Runtimeで再生するテクスチャとモーション設定を管理
	 *---------------------------------------------------------------------------------------*/
	struct HudConfig {
		std::string texturePath;   //< HUD表示に使用するテクスチャのAssetパス
		HudMotionSet enterMotion;  //< 登場時に再生するモーション設定
		HudMotionSet exitMotion;   //< 退場時に再生するモーション設定
	};

	/*-----------------------------------------------------------------------------------------
	 * BaseHud
	 * - HUDの状態遷移、モーション適用、スプライト描画を担当する基底クラス
	 * - 状態固有処理はIHudState、補間処理はHudMotionへ委譲
	 *---------------------------------------------------------------------------------------*/
	class BaseHud {
	public:
		enum class HudPhase { Enter, Stay, Exit, End };
		/** \brief HUDを未初期化状態で構築する */
		BaseHud();
		/** \brief HUDが所有するスプライトを解放する */
		virtual ~BaseHud();
		/** \brief HUDを初期化して登場状態を開始する \param moveFlags 有効にするモーションチャネル */
		void Initialize(uint32_t moveFlags = static_cast<uint32_t>(HudMotionChannel::Position) | static_cast<uint32_t>(HudMotionChannel::Scale) | static_cast<uint32_t>(HudMotionChannel::Alpha) | static_cast<uint32_t>(HudMotionChannel::Rotation));
		/** \brief 現在状態とスプライトを更新する \param dt 前フレームからの経過時間（秒） */
		virtual void Update(float dt);
		/** \brief HUD調整用GUIを表示する */
		void ShowGui();
		/** \brief HUDスプライトの描画命令を登録する \param renderer 所有権を持たないスプライト描画機能 */
		virtual void Draw(SpriteRenderer* renderer) const;
		/** \brief 登場状態へ遷移する */
		void StartEnter();
		/** \brief 滞在中のHUDを退場状態へ遷移する */
		void StartExit();
		/** \brief HUDが終了状態か判定する \return 終了状態の場合はtrue */
		bool IsFinished() const { return phase_ == HudPhase::End; }
		/** \brief 適用中のモーションを取得する \return HUDが所有するモーション */
		const HudMotion& GetMotion() const { return motion_; }
		/** \brief HUD設定を取得する \return HUDが保持する設定 */
		const HudConfig& GetConfig() const { return config_; }
		/** \brief HUD設定を差し替える \param config 新しく保持する設定 */
		void SetConfig(const HudConfig& config) { config_ = config; }

	protected:
		/** \brief 滞在状態に固有の派生HUD処理を更新する \param dt 前フレームからの経過時間（秒） */
		virtual void StayUpdate(float dt);
		/** \brief 派生HUDが表示を変更するためのスプライトを取得する \return BaseHudが所有するスプライト */
		SpriteObject2d& Sprite() const { return *spriteObj_; }
		/** \brief 登場モーション完了を派生HUDへ通知する */
		virtual void OnEnterFinished() {}
		/** \brief 退場モーション完了を派生HUDへ通知する */
		virtual void OnExitFinished() {}
		/** \brief 共通GUIより前に派生HUDの調整項目を表示する */
		virtual void TopGui() {}
		/** \brief 共通GUIより後に派生HUDの調整項目を表示する */
		virtual void DerivedGui() {}

	private:
		friend class HudEnterState;
		friend class HudStayState;
		friend class HudExitState;
		friend class HudEndState;
		/** \brief 補間済みモーション値を描画スプライトへ反映する */
		void ApplyMotionValue() const;
		/** \brief 指定状態へ遷移する \param state 遷移先の静的状態 \param phase 外部互換用の状態値 */
		void ChangeState(const IHudState& state, HudPhase phase);

		HudPhase phase_ = HudPhase::Enter;                    //< 外部APIとの互換性のため保持する現在状態
		HudConfig config_;                                    //< 表示テクスチャと登場・退場モーション設定
		HudMotion motion_;                                    //< HUD表示値を時間補間するモーション
		const IHudState* currentState_ = nullptr;             //< 所有権を持たない現在の静的状態への参照
		std::unique_ptr<SpriteObject2d> spriteObj_ = nullptr; //< HUDが所有する描画用スプライト
	};
} // namespace CalyxEngine