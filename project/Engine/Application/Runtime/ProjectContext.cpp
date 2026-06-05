#include <CalyxEngine/Project.h>

#include <cctype>

namespace Calyx {

	namespace {

		ProjectInfo& CurrentProjectStorage() {
			static ProjectInfo project;
			return project;
		}

		std::filesystem::path NormalizeContextPath(const std::filesystem::path& path) {
			std::error_code ec;
			auto normalized = std::filesystem::weakly_canonical(path, ec);
			return ec ? path.lexically_normal() : normalized;
		}

		bool IsResourcesAssetsPath(const std::filesystem::path& path) {
			const auto generic = path.generic_string();
			return generic == "Resources/Assets" ||
				   generic.rfind("Resources/Assets/", 0) == 0 ||
				   generic == "resources/assets" ||
				   generic.rfind("resources/assets/", 0) == 0;
		}

		std::filesystem::path StripResourcesAssetsPrefix(const std::filesystem::path& path) {
			std::filesystem::path result;
			bool				  skipResources = false;
			bool				  skipAssets	= false;

			for(const auto& part : path) {
				const std::string token = part.generic_string();
				std::string		  lower = token;
				for(char& c : lower) {
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				}

				if(!skipResources && lower == "resources") {
					skipResources = true;
					continue;
				}
				if(skipResources && !skipAssets && lower == "assets") {
					skipAssets = true;
					continue;
				}
				result /= part;
			}

			return result;
		}

	} // namespace

	void SetCurrentProject(const ProjectInfo& project) {
		CurrentProjectStorage() = project;
	}

	void ClearCurrentProject() {
		CurrentProjectStorage() = {};
	}

	bool HasCurrentProject() {
		return CurrentProjectStorage().IsValid();
	}

	const ProjectInfo& GetCurrentProject() {
		return CurrentProjectStorage();
	}

	std::filesystem::path GetProjectRoot() {
		if(HasCurrentProject()) {
			return NormalizeContextPath(CurrentProjectStorage().rootDirectory);
		}
		return NormalizeContextPath(std::filesystem::current_path());
	}

	std::filesystem::path GetAssetRoot() {
		if(HasCurrentProject()) {
			return ResolveProjectPath(CurrentProjectStorage(), CurrentProjectStorage().assetDirectory);
		}
		return NormalizeContextPath(std::filesystem::current_path() / "Resources" / "Assets");
	}

	std::filesystem::path ResolveAssetPath(const std::filesystem::path& path) {
		if(path.empty() || path.is_absolute()) {
			return path;
		}

		if(IsResourcesAssetsPath(path)) {
			return NormalizeContextPath(GetAssetRoot() / StripResourcesAssetsPrefix(path));
		}
		return NormalizeContextPath(GetAssetRoot() / path);
	}

	std::filesystem::path ToAssetRelativePath(const std::filesystem::path& path) {
		if(path.empty()) {
			return {};
		}

		if(!path.is_absolute()) {
			return IsResourcesAssetsPath(path) ? StripResourcesAssetsPrefix(path).lexically_normal() : path.lexically_normal();
		}

		std::error_code ec;
		auto relative = std::filesystem::relative(path, GetAssetRoot(), ec);
		return ec ? path.lexically_normal() : relative.lexically_normal();
	}

} // namespace Calyx
