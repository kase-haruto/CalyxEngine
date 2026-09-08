#pragma once

namespace CalyxEngine {
	class TextService;

	/*-----------------------------------------------------------------------------------------
	 * ITextRenderable
	 * - 現在描画中のSceneに属するText描画要求をTextServiceへ提出する
	 *---------------------------------------------------------------------------------------*/
	class ITextRenderable {
	public:
		virtual ~ITextRenderable() = default;
		virtual void SubmitText(TextService& service) const = 0;
	};
}
