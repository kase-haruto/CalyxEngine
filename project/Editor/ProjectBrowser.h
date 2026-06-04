#pragma once

#include <CalyxEngine/Project.h>

#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace CalyxEditor {

	class ProjectBrowser {
	public:
		ProjectBrowser();

		bool Draw(Calyx::ProjectInfo& outProject);

	private:
		void ReloadRecentProjects();
		void DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected);
		void DrawTemplateDetails();
		void DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected);
		void DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected);
		void DrawLocationDialog();
		void LoadIcons();

		bool LoadProject(const std::filesystem::path& path, Calyx::ProjectInfo& outProject);
		bool CreateBlankProject(Calyx::ProjectInfo& outProject);

		std::filesystem::path registryPath_;
		std::vector<Calyx::RecentProjectEntry> recentProjects_;
		std::array<char, 128> newProjectName_{};
		std::array<char, 512> newProjectDirectory_{};
		std::string statusMessage_;
		void* genericIcon_ = nullptr;
		void* folderIcon_ = nullptr;
	};

} // namespace CalyxEditor
