#pragma once

#include "Engine/Foundation/Serialization/SerializableObject.h"
#include <CalyxEngine/Project.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace CalyxEditor {

	/**
	 * @brief 新規プロジェクト作成時に利用可能なテンプレートの種類定義
	 */
	enum class ProjectTemplateType {
		Blank, //< 空のプロジェクト（最低限のソースコードのみ）
		Demo,  //< デモプロジェクト（サンプルシーンやアセットを含む）
	};

	/*-----------------------------------------------------------------------------------------
	 * ProjectBrowser
	 * - エディタ起動時に表示するプロジェクト選択画面
	 * - 最近使ったプロジェクトの表示、新規プロジェクト作成、既存プロジェクト読み込みを担当します。
	 * - ImGuiを用いてUIを描画し、新規作成時には対応するVisual Studioのソリューション（.sln）や
	 *   プロジェクトファイル（.vcxproj）、ソースファイルを自動生成します。
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief ProjectBrowserの機能を提供するクラスです。
	 */
	class ProjectBrowser {
	public:
		/**
		 * @brief プロジェクトテンプレートの基本情報
		 */
		struct TemplateInfo {
			ProjectTemplateType type;            //< テンプレートタイプ
			const char*			name;            //< 表示名 (英語表記)
			const char*			description;     //< テンプレートの説明文
			const char*			sourceDirectory; //< 生成されるソースコードの既定の格納ディレクトリ
			const char*			startupScene;    //< 起動時にロードするシーンパス
		};

		/**
		 * @brief コンストラクタ。プロジェクト一覧レジストリパスの設定やUIパラメータのロードを行います。
		 */
		ProjectBrowser();

		/**
		 * @brief プロジェクトブラウザ画面を描画するメイン関数
		 * @param outProject [out] ロードされた、または新規作成されたプロジェクト情報が格納されます
		 * @return プロジェクトが正常に決定（ロード/新規作成）した場合はtrue、表示継続中の場合はfalse
		 */
		bool Draw(Calyx::ProjectInfo& outProject);

	private:
		// --- UI描画関連メソッド ---
		
		/**
		 * @brief 最近使用したプロジェクト一覧をレジストリ等から再読込
		 */
		void ReloadRecentProjects();

		/**
		 * @brief プロジェクトテンプレート一覧のカード表示を描画
		 */
		void DrawTemplateCards();

		/**
		 * @brief 指定したテンプレート情報に基づいて個別カードを描画
		 * @param item 描画対象のテンプレート情報
		 */
		void DrawTemplateCard(const TemplateInfo& item);

		/**
		 * @brief 最近使ったプロジェクト一覧全体の領域を描画
		 */
		void DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected);

		/**
		 * @brief 個々の最近使ったプロジェクト項目（カード）を描画
		 */
		void DrawRecentProjectCard(const Calyx::RecentProjectEntry& entry, Calyx::ProjectInfo& outProject, bool& selected);

		/**
		 * @brief 選択中テンプレートの詳細情報（対象言語、初期起動シーンなど）を描画
		 */
		void DrawTemplateDetails();

		/**
		 * @brief 新規プロジェクト作成用のフォーム（保存場所やプロジェクト名入力欄）を描画
		 */
		void DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected);

		/**
		 * @brief 既存のプロジェクトファイルを開くためのファイル選択ダイアログ処理を描画
		 */
		void DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected);

		/**
		 * @brief 新規プロジェクトの作成先ディレクトリを選択するためのダイアログ処理を描画
		 */
		void DrawLocationDialog();

		/**
		 * @brief 表示に用いる共通アイコン（汎用ファイル、フォルダアイコン）をロードしてテクスチャを確保
		 */
		void LoadIcons();

		// --- プロジェクト操作関連メソッド ---
		
		/**
		 * @brief 指定されたパスの `.calyxproj` ファイルをロード
		 * @return 成功ならtrue
		 */
		bool LoadProject(const std::filesystem::path& path, Calyx::ProjectInfo& outProject);

		/**
		 * @brief 選択されたテンプレート設定をもとに、新規プロジェクトフォルダおよび関連ファイルを構築
		 * @return 成功ならtrue
		 */
		bool CreateProjectFromSelectedTemplate(Calyx::ProjectInfo& outProject);

	private:
		/**
		 * @brief 現在選択されているテンプレート情報を取得
		 */
		const TemplateInfo& GetSelectedTemplate() const;

		// --- 内部状態管理メンバ ---
		std::filesystem::path registryPath_;                     //< 最近使ったプロジェクト一覧を保存するJSONのレジストリパス
		std::vector<Calyx::RecentProjectEntry> recentProjects_; //< 最近使ったプロジェクトのキャッシュ配列
		std::array<char, 128> newProjectName_{};                 //< 新規作成するプロジェクト名の文字列バッファ
		std::array<char, 512> newProjectDirectory_{};            //< 新規作成するプロジェクトフォルダパスの文字列バッファ
		std::string statusMessage_;                              //< エラーや現在の進捗を示すステータスメッセージ
		ProjectTemplateType selectedTemplate_ = ProjectTemplateType::Blank; //< 現在選択中のテンプレートタイプ

		// --- UIテクスチャリソースポインタ ---
		void* genericIcon_ = nullptr; //< 汎用アセットファイルアイコン (ImTextureID)
		void* folderIcon_ = nullptr;  //< フォルダアイコン (ImTextureID)

		// --- レイアウト・調整用パラメータ構造体 ---
		/**
		 * @brief ProjectBrowserParamに関するデータを保持する構造体です。
		 */
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
				AddField("TemplateCardSize", templateCardSize_).Category("templates");
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

			CalyxEngine::Vector2 templateCardSize_{142.0f, 126.0f};
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
		}param_; //< UI表示用パラメータシリアライズオブジェクト
	};

} // namespace CalyxEditor
