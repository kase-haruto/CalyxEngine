#pragma once

#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <memory>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * Character2D
	 * - 2Dキャラクター基底クラス
	 * - SpriteObject2dを使用した、アニメーション遷移などを追加したクラス。
	 *---------------------------------------------------------------------------------------*/
	class Character2D {
	public:
		//===================================================================*/
		//                    public methods
		//===================================================================*/
		/**
		 * \brief コンストラクタ
		 */
		Character2D();
		/**
		 * \brief デストラクタ
		 */
		virtual ~Character2D();
		/**
		 * \brief 初期化処理
		 */
		virtual void Initialize() = 0;
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		virtual void Update(float dt);
		/**
		 * \brief GUI表示
		 */
		virtual void ShowGui() = 0;

		/**
		 * \brief 描画処理
		 * \param renderer スプライトレンダラー
		 */
		void Draw(SpriteRenderer* renderer) const;

	public:
		//===================================================================*/
		//                    accessor
		//===================================================================*/
		// getter
		/**
		 * \brief スプライトを取得
		 * \return スプライト
		 */
		Sprite*					  GetSprite() const { return spriteObj_->GetSprite(); }
		/**
		 * \brief 座標を取得
		 * \return 座標
		 */
		const CalyxEngine::Vector2& GetPosition() const { return position_; }
		/**
		 * \brief 速度を取得
		 * \return 速度
		 */
		const CalyxEngine::Vector2& GetVelocity() const { return velocity_; }
		// setter
		/**
		 * \brief 座標を設定
		 * \param pos 座標
		 */
		void SetPosition(const CalyxEngine::Vector2& pos)const { spriteObj_->SetPosition(pos); }
		/**
		 * \brief サイズを設定
		 * \param size サイズ
		 */
		void SetSize(const CalyxEngine::Vector2& size)const { spriteObj_->SetScale(size); }
		/**
		 * \brief 速度を設定
		 * \param vel 速度
		 */
		void SetVelocity(const CalyxEngine::Vector2& vel) { velocity_ = vel; }

	protected:
		std::unique_ptr<SpriteObject2d> spriteObj_ = nullptr; //< スプライトオブジェクト
		
	private:
		//===================================================================*/
		//                   private members
		//===================================================================*/
		CalyxEngine::Vector2				position_;	  //< 位置
		CalyxEngine::Vector2				velocity_;	  //< 速度
	};

} // namespace CalyxEngine
