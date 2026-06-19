#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <Engine/Foundation/Export/CalyxAPI.h>

namespace Calyx {

	struct CALYX_API ProjectInfo {
		std::string			  name;
		std::string			  engineVersion;
		std::filesystem::path projectFile;
		std::filesystem::path rootDirectory;
		std::filesystem::path assetDirectory;
		std::filesystem::path sourceDirectory;
		std::filesystem::path startupScene;
		std::filesystem::path gameModule;
		std::filesystem::path gameModuleDebug;
		std::filesystem::path gameModuleDevelop;
		std::filesystem::path gameModuleRelease;
		std::string			  launchConfiguration;
		std::string			  templateName;

		bool IsValid() const { return !projectFile.empty() && !rootDirectory.empty(); }
	};

	struct CALYX_API RecentProjectEntry {
		std::string			  name;
		std::string			  engineVersion;
		std::filesystem::path projectFile;
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
	CALYX_API std::filesystem::path GetAssetRoot();
	CALYX_API std::filesystem::path ResolveAssetPath(const std::filesystem::path& path);
	CALYX_API std::filesystem::path ToAssetRelativePath(const std::filesystem::path& path);

	CALYX_API std::filesystem::path DefaultProjectRegistryPath();
	CALYX_API bool				  LoadRecentProjects(const std::filesystem::path& path, std::vector<RecentProjectEntry>& outProjects);
	CALYX_API bool				  SaveRecentProjects(const std::filesystem::path& path, const std::vector<RecentProjectEntry>& projects);
	CALYX_API void				  AddRecentProject(std::vector<RecentProjectEntry>& projects, const ProjectInfo& project, size_t maxCount = 16);

} // namespace Calyx
