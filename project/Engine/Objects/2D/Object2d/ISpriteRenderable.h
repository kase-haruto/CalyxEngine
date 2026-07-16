#pragma once

class SpriteRenderer;

namespace CalyxEngine {

	/**
	 * @brief 2D描画対象をフレーム単位でRendererへ登録するインターフェース。
	 *
	 * オブジェクトの所有と生存期間はSceneObjectLibraryが管理する。
	 * SpriteおよびSpriteRendererの所有権は持たず、シリアライズ対象も管理しない。
	 * SpriteRendererは1回の描画処理に必要な参照だけを一時的に保持する。
	 */
	class ISpriteRenderable {
	public:
		virtual ~ISpriteRenderable() = default;

		/**
		 * @brief このオブジェクトが所有するSpriteを現在フレームの描画対象へ登録する。
		 * @param renderer 現在の描画処理中のみ有効な、非所有のSpriteRenderer参照。
		 * @return なし。
		 * @note 実装側はrendererを保持してはならない。登録したSprite参照はDraw後に
		 *       クリアされるため、描画中にSpriteを即時破棄してはならない。
		 */
		virtual void SubmitSprites(SpriteRenderer& renderer) const = 0;
	};

} // namespace CalyxEngine
