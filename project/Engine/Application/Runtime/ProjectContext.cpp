#include <CalyxEngine/Project.h>

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
	// アセットパスを実際に参照可能なパスへ解決する
	// 相対パスはアセットルートからの相対パスとして扱う
	///////////////////////////////////////////////////////////////////////////////
	std::filesystem::path ResolveAssetPath(const std::filesystem::path& path) {
		if(path.empty() || path.is_absolute()) {
			return path;
		}

		// Resources/Assets から始まるパスは、アセットルートとの二重結合を防ぐため先頭を取り除く
		if(IsResourcesAssetsPath(path)) {
			return NormalizeContextPath(GetAssetRoot() / StripResourcesAssetsPrefix(path));
		}

		return NormalizeContextPath(GetAssetRoot() / path);
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