#include <CalyxEngine/Project.h>
#include <Engine/Foundation/Log/EngineLogger.h>

#include <cctype>

namespace Calyx {

	namespace {

		///////////////////////////////////////////////////////////////////////////////
		// 現在開いているプロジェクト情報を保持する
		// static 変数として持つことで、エンジン全体から同じ ProjectInfo を参照できる
		///////////////////////////////////////////////////////////////////////////////
		ProjectInfo& CurrentProjectStorage() {
			static ProjectInfo project;
			return project;
		}

		///////////////////////////////////////////////////////////////////////////////
		// パスを可能な限り正規化する
		// weakly_canonical が失敗した場合は、文字列上の正規化のみ行う
		///////////////////////////////////////////////////////////////////////////////
		std::filesystem::path NormalizeContextPath(const std::filesystem::path& path) {
			std::error_code ec;
			auto normalized = std::filesystem::weakly_canonical(path, ec);
			return ec ? path.lexically_normal() : normalized;
		}

		///////////////////////////////////////////////////////////////////////////////
		// 指定されたパスが Resources/Assets 以下を指しているか判定する
		// 旧形式や直接指定されたアセットパスを吸収するために使用する
		///////////////////////////////////////////////////////////////////////////////
		bool IsResourcesAssetsPath(const std::filesystem::path& path) {
			const auto generic = path.generic_string();
			return generic == "Resources/Assets" ||
				   generic.rfind("Resources/Assets/", 0) == 0 ||
				   generic == "resources/assets" ||
				   generic.rfind("resources/assets/", 0) == 0;
		}

		///////////////////////////////////////////////////////////////////////////////
		// Resources/Assets の先頭部分を取り除く
		// 例: Resources/Assets/Textures/a.png -> Textures/a.png
		///////////////////////////////////////////////////////////////////////////////
		std::filesystem::path StripResourcesAssetsPrefix(const std::filesystem::path& path) {
			std::filesystem::path result;
			bool				  skipResources = false;
			bool				  skipAssets	= false;

			for(const auto& part : path) {
				const std::string token = part.generic_string();

				// 大文字小文字の違いを吸収するため、小文字に変換して比較する
				std::string lower = token;
				for(char& c : lower) {
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}

				// 先頭から Resources / Assets の順に見つかった場合は、その部分を除外する
				if(!skipResources && lower == "resources") {
					skipResources = true;
					continue;
				}
				if(skipResources && !skipAssets && lower == "assets") {
					skipAssets = true;
					continue;
				}

				// Resources/Assets 以降の要素だけを結果に追加する
				result /= part;
			}

			return result;
		}

	} // namespace

	///////////////////////////////////////////////////////////////////////////////
	// 現在のプロジェクト情報を設定する
	///////////////////////////////////////////////////////////////////////////////
	void SetCurrentProject(const ProjectInfo& project) {
		CurrentProjectStorage() = project;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在のプロジェクト情報をクリアする
	///////////////////////////////////////////////////////////////////////////////
	void ClearCurrentProject() {
		CurrentProjectStorage() = {};
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在有効なプロジェクトが設定されているか確認する
	///////////////////////////////////////////////////////////////////////////////
	bool HasCurrentProject() {
		return CurrentProjectStorage().IsValid();
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在のプロジェクト情報を取得する
	///////////////////////////////////////////////////////////////////////////////
	const ProjectInfo& GetCurrentProject() {
		return CurrentProjectStorage();
	}

	///////////////////////////////////////////////////////////////////////////////
	// .calyxproj から読み込んだプロジェクト情報を ProjectContext に保持する。
	///////////////////////////////////////////////////////////////////////////////
	bool ProjectContext::SetProjectFilePath(const std::filesystem::path& projectFilePath) {
		ProjectInfo loadedProject;
		if(!LoadProjectFile(projectFilePath, loadedProject)) {
			return false;
		}
		SetProject(loadedProject);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 既に読み込み済みの ProjectInfo を ProjectContext に保持する。
	///////////////////////////////////////////////////////////////////////////////
	void ProjectContext::SetProject(const ProjectInfo& project) {
		project_ = project;
	}

	///////////////////////////////////////////////////////////////////////////////
	// ProjectContext が有効なプロジェクト情報を保持しているか確認する。
	///////////////////////////////////////////////////////////////////////////////
	bool ProjectContext::IsValid() const {
		return project_.IsValid();
	}

	///////////////////////////////////////////////////////////////////////////////
	// ProjectContext が保持している ProjectInfo を取得する。
	///////////////////////////////////////////////////////////////////////////////
	const ProjectInfo& ProjectContext::GetProject() const {
		return project_;
	}

	///////////////////////////////////////////////////////////////////////////////
	// ProjectRoot を取得する。無効な context の場合は空パスを返す。
	///////////////////////////////////////////////////////////////////////////////
	const std::filesystem::path& ProjectContext::GetProjectRoot() const {
		static const std::filesystem::path emptyPath;
		return IsValid() ? project_.rootDirectory : emptyPath;
	}

	///////////////////////////////////////////////////////////////////////////////
	// ProjectRoot/Resources を取得する。
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path ProjectContext::GetResourcesRoot() const {
		return IsValid() ? NormalizeContextPath(project_.rootDirectory / "Resources") : std::filesystem::path{};
	}

	///////////////////////////////////////////////////////////////////////////////
	// ProjectRoot を基準に assetDirectory を解決して AssetRoot を取得する。
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path ProjectContext::GetAssetRoot() const {
		return IsValid() ? ResolveProjectPath(project_, project_.assetDirectory) : std::filesystem::path{};
	}

	///////////////////////////////////////////////////////////////////////////////
	// AssetRoot/Scenes を取得する。
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path ProjectContext::GetSceneRoot() const {
		const auto assetRoot = GetAssetRoot();
		return assetRoot.empty() ? std::filesystem::path{} : NormalizeContextPath(assetRoot / "Scenes");
	}

	///////////////////////////////////////////////////////////////////////////////
	// グローバルな現在プロジェクトを使う AssetPathResolver を生成する。
	///////////////////////////////////////////////////////////////////////////////
	AssetPathResolver::AssetPathResolver() = default;

	///////////////////////////////////////////////////////////////////////////////
	// 明示的な ProjectContext を使う AssetPathResolver を生成する。
	///////////////////////////////////////////////////////////////////////////////
	AssetPathResolver::AssetPathResolver(const ProjectContext* projectContext)
		: projectContext_(projectContext) {}

	///////////////////////////////////////////////////////////////////////////////
	// AssetRoot を基準にアセット相対パスを絶対パスへ解決する。
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path AssetPathResolver::ResolveAssetPath(const std::filesystem::path& assetRelativePath) const {
		if(assetRelativePath.empty()) {
			return {};
		}
		if(assetRelativePath.is_absolute()) {
			return NormalizeContextPath(assetRelativePath);
		}

		const std::filesystem::path assetRoot =
			projectContext_ && projectContext_->IsValid()
				? projectContext_->GetAssetRoot()
				: GetAssetRoot();
		const std::filesystem::path normalizedRelative =
			IsResourcesAssetsPath(assetRelativePath)
				? StripResourcesAssetsPrefix(assetRelativePath)
				: assetRelativePath;

		return NormalizeContextPath(assetRoot / normalizedRelative);
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセットの存在を確認し、見つからない場合は解決後パスをログへ出す。
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPathResolver::ExistsAsset(const std::filesystem::path& assetRelativePath) const {
		const auto resolvedPath = ResolveAssetPath(assetRelativePath);
		std::error_code ec;
		const bool exists = std::filesystem::exists(resolvedPath, ec);
		if(!exists || ec) {
			CalyxEngine::EngineLogger::GetInstance().Add(
				CalyxEngine::LogLevel::Warning,
				CalyxEngine::LogCategory::Asset,
				"Asset file not found: request=" + assetRelativePath.generic_string() +
					", resolved=" + resolvedPath.generic_string() +
					(ec ? ", error=" + ec.message() : std::string{}),
				"AssetPathResolver");
		}
		return exists && !ec;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在のプロジェクトルートを取得する
	// プロジェクトが設定されていない場合は、現在の作業ディレクトリを使用する
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path GetProjectRoot() {
		if(HasCurrentProject()) {
			return NormalizeContextPath(CurrentProjectStorage().rootDirectory);
		}
		return NormalizeContextPath(std::filesystem::current_path());
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在プロジェクトの ResourcesRoot を取得する。
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path GetResourcesRoot() {
		if(HasCurrentProject()) {
			return NormalizeContextPath(CurrentProjectStorage().rootDirectory / "Resources");
		}
		return NormalizeContextPath(std::filesystem::current_path() / "Resources");
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセットルートディレクトリを取得する
	// プロジェクトが設定されている場合は ProjectInfo の assetDirectory を基準にする
	// 未設定の場合は Resources/Assets をデフォルトのアセットルートとして扱う
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path GetAssetRoot() {
		if(HasCurrentProject()) {
			return ResolveProjectPath(CurrentProjectStorage(), CurrentProjectStorage().assetDirectory);
		}
		return NormalizeContextPath(std::filesystem::current_path() / "Resources" / "Assets");
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在プロジェクトの SceneRoot を取得する。
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path GetSceneRoot() {
		return NormalizeContextPath(GetAssetRoot() / "Scenes");
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセットパスを実際に参照可能なパスへ解決する
	// 相対パスはアセットルートからの相対パスとして扱う
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path ResolveAssetPath(const std::filesystem::path& path) {
		if(path.empty() || path.is_absolute()) {
			return AssetPathResolver().ResolveAssetPath(path);
		}

		// Resources/Assets から始まるパスは、アセットルートとの二重結合を防ぐため先頭を取り除く
		if(IsResourcesAssetsPath(path)) {
			return AssetPathResolver().ResolveAssetPath(path);
		}

		return AssetPathResolver().ResolveAssetPath(path);
	}

	///////////////////////////////////////////////////////////////////////////////
	// 現在プロジェクトの AssetRoot を基準にアセットの存在を確認する。
	///////////////////////////////////////////////////////////////////////////////
	bool ExistsAsset(const std::filesystem::path& path) {
		return AssetPathResolver().ExistsAsset(path);
	}

	///////////////////////////////////////////////////////////////////////////////
	// 任意のパスをアセットルートからの相対パスへ変換する
	// 保存データやアセット参照では、この形式にしておくとプロジェクト移動に強くなる
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path ToAssetRelativePath(const std::filesystem::path& path) {
		if(path.empty()) {
			return {};
		}

		// すでに相対パスの場合はそのまま使う
		// Resources/Assets から始まる場合は、その部分を取り除いてアセット相対にする
		if(!path.is_absolute()) {
			return IsResourcesAssetsPath(path) ? StripResourcesAssetsPrefix(path).lexically_normal() : path.lexically_normal();
		}

		// 絶対パスの場合は、アセットルートからの相対パスに変換する
		std::error_code ec;
		auto relative = std::filesystem::relative(path, GetAssetRoot(), ec);
		return ec ? path.lexically_normal() : relative.lexically_normal();
	}

} // namespace Calyx
