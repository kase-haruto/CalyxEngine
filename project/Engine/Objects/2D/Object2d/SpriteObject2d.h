#pragma once

// engine
#include <Engine/Foundation/Math/Vector2.h>

// stde
#include "Engine/Foundation/Math/Vector4.h"

#include <cstdint>
#include <memory>
#include <string>

class SpriteRenderer;
class Sprite;

namespace Calyx2D {

	//==============================================
	// アニメーション構造体（1スプライトシート用）
	//==============================================
	struct SpriteAnimation {
		std::string					texturePath;
		std::pair<int32_t, int32_t> division{1, 1};

		int32_t startFrame	  = 0;
		int32_t frameCount	  = 0; // 0 → 自動で division.x * division.y
		float	frameDuration = 0.1f;
		bool	loop		  = true;
	};

	/*-----------------------------------------------------------------------------------------
	 * SpriteObject2d
	 * - 2Dスプライトオブジェクトクラス
	 * - spriteクラスを使いやすくしたラッパ
	 * - uvアニメーションでスプライトのアニメーションなどが可能
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

		//====================================================
		// Animation API
		//====================================================
		/**
		 * \brief アニメーション追加
		 * \param name アニメーション名
		 * \param anim アニメーションデータ
		 */
		void AddAnimation(const std::string& name, SpriteAnimation anim);
		/**
		 * \brief アニメーション設定
		 * \param name アニメーション名
		 * \param restart 再生を最初からにするか
		 * \return 成功したらtrue
		 */
		bool SetAnimation(const std::string& name, bool restart = true);

	public:
		// getter
		const std::pair<int32_t, int32_t>& GetDivision() const;
		const CalyxMath::Vector2&		   GetPosition() const;
		const CalyxMath::Vector2&		   GetScale() const;
		Sprite*							   GetSprite() const;
		CalyxMath::Vector2 GetUvTranslate() const;
		float GetUvRotate() const;

		// setter
		void SetDivision(const std::pair<int32_t, int32_t>& division);
		void SetPosition(const CalyxMath::Vector2& position) const;
		void SetScale(const CalyxMath::Vector2& scale) const;
		void SetRotation(float rotation) const;
		void SetAlpha(float alpha) const;
		void SetColor(const CalyxMath::Vector4 & color) const;
		void SetVisibility(bool visible) const;
		void SetUvTranslate(const CalyxMath::Vector2& uv) const;
		void SetUvRotate(float rot) const;
		void SetUvOffset(const CalyxMath::Vector2& offset) const; // 加算したいなら
		void SetFrameDuration(float duration) ;

	private:
		/** \brief アニメーション更新処理
		 * \param dt デルタタイム
		 */
		void AnimationUpdate(float dt);
		/**
		 * \brief フレームをUVに反映する
		 */
		void ApplyFrameToUv() const;

	private:
		std::unique_ptr<Sprite> sprite_ = nullptr;

		// 現在使用中の division
		std::pair<int32_t, int32_t> division_ = {1, 1};

		// アニメーション用
		std::unordered_map<std::string, SpriteAnimation> animations_;
		const SpriteAnimation*							 currentAnim_ = nullptr;
		std::string										 currentAnimName_;

		// フレーム制御
		int32_t currentFrame_  = 0;
		float	frameTime_	   = 0.0f;
		float	frameDuration_ = 0.1f;
	};

} // namespace Calyx2D