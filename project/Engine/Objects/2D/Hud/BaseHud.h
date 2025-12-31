#pragma once
/* ==========================================================================
 *	include space
 * ========================================================================*/
#include <Engine/Foundation/Utility/Animation/SimpleAnimation.h>
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>

namespace Calyx2D {

	/*-----------------------------------------------------------------------------------------
	 * HUD設定構造体
	 * - HUDの登場・退場の設定をまとめた構造体
	 *---------------------------------------------------------------------------------------*/
	struct HudConfig {
		std::string		   texturePath; //< テクスチャパス
		CalyxMath::Vector2 startPos_;	//< 開始位置
		CalyxMath::Vector2 stayPos_;	//< ベース位置
		CalyxMath::Vector2 endPos_;		//< 終了位置

		float enterDuration_ = 0.5f; //< 登場時間
		float exitDuration_	 = 0.5f; //< 退場時間
	};

	/*-----------------------------------------------------------------------------------------
	 * HUD基底クラス
	 * - object2dを使用してhudを構築する基底クラス
	 * - 登場や退場をイージングで制御する機能を持つ
	 * - 使用するイージングは派生先で設定させる
	 *---------------------------------------------------------------------------------------*/
	class BaseHud {
		//===================================================================*/
		//                    structs / enums
		//===================================================================*/
		enum class HudPhase {
			Stay,  //< 待機
			Enter, //< 登場
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
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		void Initialize(const HudConfig& config);
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		virtual void Update(float dt);
		/**
		 * \brief 描画処理
		 * \param renderer レンダラー
		 */
		void Draw(SpriteRenderer* renderer) const;
		/**
		 * \brief HUDの退場を開始
		 */
		void StartExit();
		/**
		 * \brief HUDの登場を開始
		 */
		void StartEnter();

		//===================================================================*/
		//                    accessor
		//===================================================================*/
		bool IsFinished() const { return phase_ == HudPhase::End; }

	protected:
		//===================================================================*/
		//                    protected methods
		//===================================================================*/
		/**
		 * \brief 登場中の更新処理
		 */
		virtual void StayUpdate(float dt);
		/**
		 * \brief スプライトオブジェクト取得
		 * \return スプライトの参照
		 */
		SpriteObject2d& Sprite()const { return *spriteObj_; }
		/**
		 * \brief 登場完了時の処理
		 */
		virtual void OnEnterFinished() {}
		/**
		 * \brief 退場完了時の処理
		 */
		virtual void OnExitFinished() {}

	protected:
		//===================================================================*/
		//                    protected methods
		//===================================================================*/

		HudPhase  phase_ = HudPhase::Enter; //< フェーズ
		HudConfig config_;					//< HUD設定
		float	  time_ = 0.0f;				//< 経過時間

		// アニメーション
		CalyxUtil::SimpleAnimation<CalyxMath::Vector2> moveAnim_;

	private:
		//===================================================================*/
		//                    private methods
		//===================================================================*/
		std::unique_ptr<SpriteObject2d> spriteObj_ = nullptr; //< スプライトオブジェクト
	};

} // namespace Calyx2D
