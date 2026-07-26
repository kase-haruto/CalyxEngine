#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Engine/Foundation/Export/CalyxAPI.h>

namespace Calyx {

	/*-----------------------------------------------------------------------------------------
	 * ProjectInfo
	 * - Calyxプロジェクトの構成情報を保持するデータ構造
	 * - プロジェクトルート、アセット配置、ゲームモジュールと起動構成を管理する
	 * - ファイルの読み書きやアセットのロード処理は担当しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief ProjectInfoに関するデータを保持する構造体です。
	 */
	struct CALYX_API ProjectInfo {
		std::string			  name;
		std::string			  engineVersion;
		std::filesystem::path projectFile;
		std::filesystem::path rootDirectory;
		std::filesystem::path assetDirectory;
		std::filesystem::path sourceDirectory;
		std::filesystem::path generatedDirectory = "Generated";
		std::filesystem::path startupScene;
		std::filesystem::path gameModule;
		std::filesystem::path gameModuleDebug;
		std::filesystem::path gameModuleDevelop;
		std::filesystem::path gameModuleRelease;
		std::string			  launchConfiguration;
		std::string			  templateName;

		bool IsValid() const { return !projectFile.empty() && !rootDirectory.empty(); }
	};

	/*-----------------------------------------------------------------------------------------
	 * RecentProjectEntry
	 * - 最近使用したプロジェクト一覧へ保存する軽量データ構造
	 * - 表示名、Engineバージョン、プロジェクトファイルの場所を保持する
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief RecentProjectEntryに関するデータを保持する構造体です。
	 */
	struct CALYX_API RecentProjectEntry {
		std::string			  name;
		std::string			  engineVersion;
		std::filesystem::path projectFile;
	};

	/*-----------------------------------------------------------------------------------------
	 * ProjectContext
	 * - 現在開いているプロジェクトファイルとルートディレクトリ情報を保持する。
	 * - ProjectRoot / ResourcesRoot / AssetRoot / SceneRoot を提供する。
	 * - ゲーム固有アセットの中身やロード処理は管理しない。
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief ProjectContextの機能を提供するクラスです。
	 */
	class CALYX_API ProjectContext {
	public:
		/**
		 * \brief .calyxproj のパスからプロジェクト情報を読み込み、このコンテキストへ保持する。
		 * \param projectFilePath .calyxproj ファイルのパス。
		 * \return 読み込みと保持に成功した場合 true。
		 * \note パスの正規化は LoadProjectFile 側で行う。
		 */
		bool SetProjectFilePath(const std::filesystem::path& projectFilePath);

		/**
		 * \brief 既に読み込み済みのプロジェクト情報を保持する。
		 * \param project 保持する ProjectInfo。
		 */
		void SetProject(const ProjectInfo& project);

		/**
		 * \brief 有効なプロジェクト情報を保持しているか確認する。
		 * \return projectFile と rootDirectory が設定されている場合 true。
		 */
		bool IsValid() const;

		/**
		 * \brief 保持しているプロジェクト情報を取得する。
		 * \return このコンテキストが保持する ProjectInfo。
		 */
		const ProjectInfo& GetProject() const;

		/**
		 * \brief プロジェクトルートディレクトリを取得する。
		 * \return 有効な場合は絶対 ProjectRoot、無効な場合は空パス。
		 */
		const std::filesystem::path& GetProjectRoot() const;

		/**
		 * \brief Resources ルートディレクトリを取得する。
		 * \return 有効な場合は ProjectRoot/Resources、無効な場合は空パス。
		 */
		std::filesystem::path GetResourcesRoot() const;

		/**
		 * \brief Asset ルートディレクトリを取得する。
		 * \return ProjectRoot を基準に project.assetDirectory を解決したパス。
		 */
		std::filesystem::path GetAssetRoot() const;

		/**
		 * \brief Scene ルートディレクトリを取得する。
		 * \return 有効な場合は AssetRoot/Scenes、無効な場合は空パス。
		 */
		std::filesystem::path GetSceneRoot() const;

	private:
		ProjectInfo project_;
	};

	/*-----------------------------------------------------------------------------------------
	 * AssetPathResolver
	 * - 現在のプロジェクトの AssetRoot を基準にアセット相対パスを絶対パスへ解決する。
	 * - 旧形式の "Resources/Assets/..." 入力も受け取り、AssetRoot 相対へ正規化する。
	 * - アセット種別の判定やファイル内容のロードは行わない。
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief AssetPathResolverの機能を提供するクラスです。
	 */
	class CALYX_API AssetPathResolver {
	public:
		/**
		 * \brief グローバルな現在プロジェクトを使う resolver を生成する。
		 */
		AssetPathResolver();

		/**
		 * \brief 明示的な ProjectContext を使う resolver を生成する。
		 * \param projectContext 解決基準にする ProjectContext。nullptr の場合はグローバルな現在プロジェクトを使う。
		 */
		explicit AssetPathResolver(const ProjectContext* projectContext);

		/**
		 * \brief アセット相対パスを絶対パスへ解決する。
		 * \param assetRelativePath AssetRoot 相対パス、または旧形式の Resources/Assets パス。
		 * \return 相対入力の場合は絶対パス、絶対入力の場合は正規化したパス。
		 */
		std::filesystem::path ResolveAssetPath(const std::filesystem::path& assetRelativePath) const;

		/**
		 * \brief アセット相対パスが存在するか確認する。
		 * \param assetRelativePath AssetRoot 相対パス、または旧形式の Resources/Assets パス。
		 * \return 解決後のファイルが存在する場合 true。
		 * \note 見つからない場合は、読み込み側がクラッシュせず失敗できるようログへ出力する。
		 */
		bool ExistsAsset(const std::filesystem::path& assetRelativePath) const;

	private:
		const ProjectContext* projectContext_ = nullptr; //< 明示的に指定されたプロジェクトコンテキスト。
	};

	CALYX_API bool LoadProjectFile(const std::filesystem::path& path, ProjectInfo& outProject);
	CALYX_API bool SaveProjectFile(const ProjectInfo& project);
	CALYX_API bool CreateProject(const ProjectInfo& project);
	CALYX_API std::filesystem::path ResolveProjectPath(const ProjectInfo& project, const std::filesystem::path& path);

	CALYX_API void SetCurrentProject(const ProjectInfo& project);
	CALYX_API void ClearCurrentProject();
	CALYX_API bool HasCurrentProject();
	CALYX_API const ProjectInfo& GetCurrentProject();
	CALYX_API std::filesystem::path GetProjectRoot();
	CALYX_API std::filesystem::path GetResourcesRoot();
	CALYX_API std::filesystem::path GetAssetRoot();
	CALYX_API std::filesystem::path GetSceneRoot();
	CALYX_API std::filesystem::path ResolveAssetPath(const std::filesystem::path& path);
	CALYX_API bool ExistsAsset(const std::filesystem::path& path);
	CALYX_API std::filesystem::path ToAssetRelativePath(const std::filesystem::path& path);

	CALYX_API std::filesystem::path DefaultProjectRegistryPath();
	CALYX_API bool				  LoadRecentProjects(const std::filesystem::path& path, std::vector<RecentProjectEntry>& outProjects);
	CALYX_API bool				  SaveRecentProjects(const std::filesystem::path& path, const std::vector<RecentProjectEntry>& projects);
	CALYX_API void				  AddRecentProject(std::vector<RecentProjectEntry>& projects, const ProjectInfo& project, size_t maxCount = 16);

} // namespace Calyx
