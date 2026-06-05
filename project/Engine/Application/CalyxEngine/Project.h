#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace Calyx {

	struct ProjectInfo {
		std::string			  name;
		std::string			  engineVersion;
		std::filesystem::path projectFile;
		std::filesystem::path rootDirectory;
		std::filesystem::path assetDirectory;
		std::filesystem::path sourceDirectory;
		std::filesystem::path startupScene;
		std::string			  templateName;

		bool IsValid() const { return !projectFile.empty() && !rootDirectory.empty(); }
	};

	struct RecentProjectEntry {
		std::string			  name;
		std::string			  engineVersion;
		std::filesystem::path projectFile;
	};

	bool LoadProjectFile(const std::filesystem::path& path, ProjectInfo& outProject);
	bool SaveProjectFile(const ProjectInfo& project);
	bool CreateProject(const ProjectInfo& project);
	std::filesystem::path ResolveProjectPath(const ProjectInfo& project, const std::filesystem::path& path);

	void SetCurrentProject(const ProjectInfo& project);
	void ClearCurrentProject();
	bool HasCurrentProject();
	const ProjectInfo& GetCurrentProject();
	std::filesystem::path GetProjectRoot();
	std::filesystem::path GetAssetRoot();
	std::filesystem::path ResolveAssetPath(const std::filesystem::path& path);
	std::filesystem::path ToAssetRelativePath(const std::filesystem::path& path);

	std::filesystem::path DefaultProjectRegistryPath();
	bool				  LoadRecentProjects(const std::filesystem::path& path, std::vector<RecentProjectEntry>& outProjects);
	bool				  SaveRecentProjects(const std::filesystem::path& path, const std::vector<RecentProjectEntry>& projects);
	void				  AddRecentProject(std::vector<RecentProjectEntry>& projects, const ProjectInfo& project, size_t maxCount = 16);

} // namespace Calyx
