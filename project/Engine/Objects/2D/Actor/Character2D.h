#pragma once

#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <memory>

namespace Calyx2D {

	/*-----------------------------------------------------------------------------------------
	 * 2Dキャラクター基底クラス
	 * - SpriteObject2dを使用した、アニメーション遷移などを追加したクラス。
	 *---------------------------------------------------------------------------------------*/
	class Character2D {
	public:
		//===================================================================*/
		//                    public methods
		//===================================================================*/
		/**
		 * \brief コンストラクタ / デストラクタ
		 */
		Character2D();
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

	public:
		//===================================================================*/
		//                    accessor
		//===================================================================*/
		// getter
		Sprite*					  GetSprite() const { return spriteObj_->GetSprite(); }
		const CalyxMath::Vector2& GetPosition() const { return position_; }
		const CalyxMath::Vector2& GetDirection() const { return direction_; }
		const CalyxMath::Vector2& GetVelocity() const { return velocity_; }
		// setter
		void SetPosition(const CalyxMath::Vector2& pos) { position_ = pos; }
		void SetDirection(const CalyxMath::Vector2& dir) { direction_ = dir; }
		void SetVelocity(const CalyxMath::Vector2& vel) { velocity_ = vel; }

	protected:
		//===================================================================*/
		//                   private members
		//===================================================================*/
		std::unique_ptr<SpriteObject2d> spriteObj_ = nullptr; //< スプライトオブジェクト
		CalyxMath::Vector2				position_;			  //< 位置
		CalyxMath::Vector2				direction_;			  //< 方向
		CalyxMath::Vector2				velocity_;			  //< 速度
	};

} // namespace Calyx2D
