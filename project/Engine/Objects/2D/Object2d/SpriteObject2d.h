#pragma once

// engine
#include <Engine/Foundation/Math/Vector2.h>

// stde
#include "Engine/Foundation/Math/Vector4.h"

#include <memory>
#include <string>

class SpriteRenderer;
class Sprite;

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * SpriteObject2d
	 * - 2Dスプライトオブジェクトクラス
	 * - spriteクラスを使いやすくしたラッパ
	 *---------------------------------------------------------------------------------------*/
	class SpriteObject2d {
	public:
		/** \brief コンストラクタ・デストラクタ */
		SpriteObject2d();
		~SpriteObject2d();

		/**
		 * \brief 初期化処理
		 * \param filePath
		 */
		void Initialize(const std::string& filePath);
		/**
		 * \brief 更新処理
		 * \param dt デルタタイム
		 */
		void Update(float dt);
		/**
		 * \brief 描画処理
		 * \param renderer レンダラー
		 */
		void Draw(SpriteRenderer* renderer) const;

		/**
		 * \brief デバッグGui
		 */
		void ShowGui();

	public:
		// getter
		const CalyxEngine::Vector2&		   GetPosition() const;
		const CalyxEngine::Vector2&		   GetScale() const;
		Sprite*							   GetSprite() const;
		CalyxEngine::Vector2 GetUvTranslate() const;
		float GetUvRotate() const;

		// setter
		void SetTexture(const std::string& texturePath) const;
		void SetPosition(const CalyxEngine::Vector2& position) const;
		void SetScale(const CalyxEngine::Vector2& scale) const;
		void SetRotation(float rotation) const;
		void SetAlpha(float alpha) const;
		void SetColor(const CalyxEngine::Vector4 & color) const;
		void SetVisibility(bool visible) const;
		void SetAnchorPoint(const CalyxEngine::Vector2& anchor) const;
		void SetUvTranslate(const CalyxEngine::Vector2& uv) const;
		void SetUvRotate(float rot) const;
		void SetUvScale(const CalyxEngine::Vector2& scale) const;
		void SetUvOffset(const CalyxEngine::Vector2& offset) const; // 加算したいなら

	private:
		std::unique_ptr<Sprite> sprite_ = nullptr;
	};

} // namespace CalyxEngine
