#pragma once

#include <Engine\Application\UI\EngineUI\IEngineUI.h>
#include <Engine\Foundation\Utility\Guid\Guid.h>

#include <filesystem>

namespace CalyxEngine {
	class SpriteAnimationAsset;

	/**
	 * @brief SpriteAnimationEditorPanelの機能を提供するクラスです。
	 */
	class SpriteAnimationEditorPanel : public IEngineUI {
	public:
		SpriteAnimationEditorPanel();
		void Render() override;

	private:
		void DrawAssetList();
		void DrawEditor(SpriteAnimationAsset& asset, const std::filesystem::path& path);
		void DrawTexture(SpriteAnimationAsset& asset);
		void DrawClips(SpriteAnimationAsset& asset);
		void CreateSpriteAnimationAsset();
		void Save(SpriteAnimationAsset& asset, const std::filesystem::path& path);

	private:
		Guid selectedAnimation_;
		int selectedClipIndex_ = 0;
	};

} // namespace CalyxEngine
