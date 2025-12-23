#pragma once

// engine
#include <Engine/Foundation/Math/Vector2.h>

// stde
#include <cstdint>
#include <d3d12.h>
#include <memory>
#include <string>

class Sprite;

namespace Calyx2D {

	/*----------------------------------------------------------------------------------------
	 * 2Dスプライトオブジェクトクラス
	 * - spriteクラスを使いやすくしたラッパ
	 * - uvアニメーションでスプライトのアニメーションなどが可能
	 *---------------------------------------------------------------------------------------*/
	class SpriteObject2d {
	public:
		//===================================================================*/
		//			public methods
		//===================================================================*/
		/** \brief コンストラクタ・デストラクタ */
		SpriteObject2d();
		~SpriteObject2d();

		/**
		 * \brief 初期化
		 * \param filePath ファイルパス(Textures/以下からの相対パス)
		 */
		void Initializes(const std::string& filePath);
		/**
		 * \brief 更新
		 */
		void Update(float dt) const;
		/**
		 * \brief 描画
		 * \param cmdList コマンドリスト
		 */
		void Draw(ID3D12GraphicsCommandList* cmdList) const;

	public:
		//===================================================================*/
		//			accessor
		//===================================================================*/
		// getter
		const std::pair<int32_t, int32_t>& GetDivision() const;
		const CalyxMath::Vector2&		   GetPosition() const;
		const CalyxMath::Vector2&		   GetScale() const;
		Sprite* GetSprite() const;
		// setter
		void SetDivision(const std::pair<int32_t, int32_t>& division);
		void SetPosition(const CalyxMath::Vector2& position) const;
		void SetScale(const CalyxMath::Vector2& scale) const;

	private:
		//===================================================================*/
		//			private methods
		//===================================================================*/
		/**
		 * \brief アニメーション更新
		 */
		void AnimationUpdate(float dt) const;

	private:
		//===================================================================*/
		//			private members
		//===================================================================*/
		std::unique_ptr<Sprite>		sprite_			   = nullptr;	//< スプライト本体
		std::pair<int32_t, int32_t> division_		   = {1, 1};	//< スプライト分割数
		mutable int32_t				currentFrame_	   = 0;			//< 現在のフレーム
		mutable float				frameTime_		   = 0.0f;		//< フレーム経過時間
		float						frameDuration_	   = 0.1f;		// 1フレームあたり0.1秒 (10fps)
	};

} // namespace Calyx2D
