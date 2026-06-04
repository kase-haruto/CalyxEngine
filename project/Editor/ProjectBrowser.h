#pragma once

#include "Engine/Foundation/Serialization/SerializableObject.h"
#include <CalyxEngine/Project.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * ProjectBrowser
	 * - エディタ起動時に表示するプロジェクト選択画面
	 * - 最近使ったプロジェクトの表示、新規プロジェクト作成、既存プロジェクト読み込みを担当
	 *---------------------------------------------------------------------------------------*/
	class ProjectBrowser {
	public:
		ProjectBrowser();

		bool Draw(Calyx::ProjectInfo& outProject);

	private:
		// --- 描画 ---
		void ReloadRecentProjects();
		void DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected);
		void DrawTemplateDetails();
		void DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected);
		void DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected);
		void DrawLocationDialog();
		void LoadIcons();

		// --- プロジェクト操作 ---
		bool LoadProject(const std::filesystem::path& path, Calyx::ProjectInfo& outProject);
		bool CreateBlankProject(Calyx::ProjectInfo& outProject);

	private:
		// --- 状態 ---
		std::filesystem::path registryPath_;
		std::vector<Calyx::RecentProjectEntry> recentProjects_;
		std::array<char, 128> newProjectName_{};
		std::array<char, 512> newProjectDirectory_{};
		std::string statusMessage_;

		// --- UIリソース ---
		void* genericIcon_ = nullptr;
		void* folderIcon_ = nullptr;

		// --- パラメータ ---
		struct ProjectBrowserParam : CalyxEngine::SerializableObject {
			ProjectBrowserParam() {
				AddField("OpenButtonSize", openButtonSize_).Category("top");
				AddField("RefreshButtonSize", refreshButtonSize_).Category("top");
				AddField("TableColumnCount", tableColumnCount_).Category("layout");
				AddField("FooterHeight", footerHeight_).Category("layout");
				AddField("RecentColumnWeight", recentColumnWeight_).Category("layout");
				AddField("DetailsColumnWeight", detailsColumnWeight_).Category("layout");
				AddField("CardSize", cardSize_).Category("recent");
				AddField("CardPadding", cardPadding_).Category("recent");
				AddField("CardImageWidthOffset", cardImageWidthOffset_).Category("recent");
				AddField("CardImageHeight", cardImageHeight_).Category("recent");
				AddField("CardNameOffsetY", cardNameOffsetY_).Category("recent");
				AddField("CardVersionOffsetY", cardVersionOffsetY_).Category("recent");
				AddField("CardBorderColor", cardBorderColor_).Category("recent");
				AddField("TemplatePreviewMaxHeight", templatePreviewMaxHeight_).Category("details");
				AddField("TemplatePreviewAspect", templatePreviewAspect_).Category("details");
				AddField("TemplateValueOffsetX", templateValueOffsetX_).Category("details");

				AddField("LabelWidth", labelWidth_).Category("newProject");
				AddField("ButtonWidth", buttonWidth_).Category("newProject");
				AddField("BrowseSize", browseSize_).Category("newProject");
				AddField("FolderIconSize", folderIconSize_).Category("newProject");
				AddField("FallbackBrowseButtonSize", fallbackBrowseButtonSize_).Category("newProject");
				AddField("CreateButtonHeight", createButtonHeight_).Category("newProject");
				AddField("MinInputWidth", minInputWidth_).Category("newProject");
				AddField("InputSpacingCount", inputSpacingCount_).Category("newProject");
			}
			CalyxEngine::ParamPath GetParamPath() const override {
				return {CalyxEngine::ParamDomain::Editor,"ProjectBrowser"};
			}
			
			CalyxEngine::Vector2 openButtonSize_{ 140.0f, 28.0f };
			CalyxEngine::Vector2 refreshButtonSize_{ 100.0f, 28.0f };
			int tableColumnCount_ = 2;
			float footerHeight_ = 128.0f;
			float recentColumnWeight_ = 0.68f;
			float detailsColumnWeight_ = 0.32f;

			CalyxEngine::Vector2 cardSize_{142.0f, 126.0f};
			float cardPadding_ = 10.0f;
			float cardImageWidthOffset_ = 20.0f;
			float cardImageHeight_ = 64.0f;
			float cardNameOffsetY_ = 78.0f;
			float cardVersionOffsetY_ = 102.0f;
			CalyxEngine::Vector4 cardBorderColor_{0.27f, 0.27f, 0.27f, 1.0f};

			float templatePreviewMaxHeight_ = 120.0f;
			float templatePreviewAspect_ = 0.52f;
			float templateValueOffsetX_ = 120.0f;

			// newProject
			float labelWidth_	= 116.0f;
			float buttonWidth_	= 104.0f;
			float browseSize_	= 28.0f;
			float folderIconSize_ = 22.0f;
			CalyxEngine::Vector2 fallbackBrowseButtonSize_{30.0f, 24.0f};
			float createButtonHeight_ = 30.0f;
			float minInputWidth_ = 180.0f;
			float inputSpacingCount_ = 3.0f;
		}param_;
	};

} // namespace CalyxEditor
