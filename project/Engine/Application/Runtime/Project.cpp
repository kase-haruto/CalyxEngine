#include <CalyxEngine/Project.h>

#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <utility>

namespace Calyx {

	namespace {

		std::filesystem::path NormalizePath(const std::filesystem::path& path) {
			std::error_code ec;
			auto normalized = std::filesystem::weakly_canonical(path, ec);
			return ec ? path.lexically_normal() : normalized;
		}

		std::filesystem::path ReadPath(
			const nlohmann::json& root,
			const char*			  key,
			const std::filesystem::path& fallback) {

			if(!root.contains(key) || !root.at(key).is_string()) {
				return fallback;
			}
			return root.at(key).get<std::string>();
		}

		std::string WritePath(const std::filesystem::path& path) {
			return path.generic_string();
		}

		std::filesystem::path RelativeProjectPath(const ProjectInfo& project, const std::filesystem::path& path) {
			if(path.empty()) return {};
			if(!path.is_absolute()) return path.lexically_normal();

			std::error_code ec;
			auto relative = std::filesystem::relative(path, project.rootDirectory, ec);
			return ec ? path.lexically_normal() : relative.lexically_normal();
		}

	} // namespace

	bool LoadProjectFile(const std::filesystem::path& path, ProjectInfo& outProject) {
		std::ifstream file(path);
		if(!file) {
			return false;
		}

		nlohmann::json root;
		try {
			file >> root;
		} catch(const nlohmann::json::exception&) {
			return false;
		}

		const auto projectFile = NormalizePath(path);
		const auto projectRoot = projectFile.parent_path();

		ProjectInfo project;
		project.name			= root.value("name", projectRoot.filename().string());
		project.engineVersion	= root.value("engineVersion", std::string{});
		project.projectFile		= projectFile;
		project.rootDirectory	= projectRoot;
		project.assetDirectory	= ReadPath(root, "assetDirectory", "Resources/Assets");
		project.sourceDirectory	= ReadPath(root, "sourceDirectory", "Game");
		project.startupScene		= ReadPath(root, "startupScene", std::filesystem::path{});
		project.templateName		= root.value("template", std::string{"Blank"});

		outProject = std::move(project);
		return true;
	}

	bool SaveProjectFile(const ProjectInfo& project) {
		if(project.projectFile.empty()) {
			return false;
		}

		std::error_code ec;
		const auto projectFileParent = project.projectFile.parent_path();
		if(!projectFileParent.empty()) {
			std::filesystem::create_directories(projectFileParent, ec);
		}
		if(ec) {
			return false;
		}

		nlohmann::json root;
		root["name"]			 = project.name;
		root["engineVersion"]	 = project.engineVersion;
		root["assetDirectory"]	 = WritePath(RelativeProjectPath(project, project.assetDirectory));
		root["sourceDirectory"]	 = WritePath(RelativeProjectPath(project, project.sourceDirectory));
		root["startupScene"]		 = WritePath(RelativeProjectPath(project, project.startupScene));
		root["template"]			 = project.templateName.empty() ? "Blank" : project.templateName;

		std::ofstream file(project.projectFile);
		if(!file) {
			return false;
		}
		file << root.dump(2);
		return true;
	}

	bool CreateProject(const ProjectInfo& project) {
		if(project.projectFile.empty() || project.rootDirectory.empty()) {
			return false;
		}

		std::error_code ec;
		std::filesystem::create_directories(project.rootDirectory, ec);
		if(ec) return false;

		std::filesystem::create_directories(ResolveProjectPath(project, project.assetDirectory), ec);
		if(ec) return false;

		std::filesystem::create_directories(ResolveProjectPath(project, project.sourceDirectory), ec);
		if(ec) return false;

		return SaveProjectFile(project);
	}

	std::filesystem::path ResolveProjectPath(const ProjectInfo& project, const std::filesystem::path& path) {
		if(path.empty() || path.is_absolute()) {
			return path;
		}
		return NormalizePath(project.rootDirectory / path);
	}

	std::filesystem::path DefaultProjectRegistryPath() {
		char*  localAppData = nullptr;
		size_t length		 = 0;
		if(_dupenv_s(&localAppData, &length, "LOCALAPPDATA") == 0 && localAppData) {
			std::filesystem::path path = std::filesystem::path(localAppData) / "CalyxEngine" / "projects.json";
			std::free(localAppData);
			return path;
		}
		return std::filesystem::path("CalyxEngine") / "projects.json";
	}

	bool LoadRecentProjects(const std::filesystem::path& path, std::vector<RecentProjectEntry>& outProjects) {
		outProjects.clear();

		std::ifstream file(path);
		if(!file) {
			return true;
		}

		nlohmann::json root;
		try {
			file >> root;
		} catch(const nlohmann::json::exception&) {
			return false;
		}
		if(!root.contains("projects") || !root.at("projects").is_array()) {
			return false;
		}

		for(const auto& item : root.at("projects")) {
			if(!item.is_object() || !item.contains("projectFile") || !item.at("projectFile").is_string()) {
				continue;
			}

			RecentProjectEntry entry;
			entry.name			 = item.value("name", std::string{});
			entry.engineVersion	 = item.value("engineVersion", std::string{});
			entry.projectFile	 = item.at("projectFile").get<std::string>();
			outProjects.push_back(std::move(entry));
		}

		return true;
	}

	bool SaveRecentProjects(const std::filesystem::path& path, const std::vector<RecentProjectEntry>& projects) {
		std::error_code ec;
		const auto registryParent = path.parent_path();
		if(!registryParent.empty()) {
			std::filesystem::create_directories(registryParent, ec);
		}
		if(ec) {
			return false;
		}

		nlohmann::json root;
		root["projects"] = nlohmann::json::array();
		for(const auto& project : projects) {
			root["projects"].push_back({
				{"name", project.name},
				{"engineVersion", project.engineVersion},
				{"projectFile", project.projectFile.generic_string()},
			});
		}

		std::ofstream file(path);
		if(!file) {
			return false;
		}
		file << root.dump(2);
		return true;
	}

	void AddRecentProject(std::vector<RecentProjectEntry>& projects, const ProjectInfo& project, size_t maxCount) {
		const auto projectFile = NormalizePath(project.projectFile);
		projects.erase(
			std::remove_if(
				projects.begin(),
				projects.end(),
				[&projectFile](const RecentProjectEntry& entry) {
					return NormalizePath(entry.projectFile) == projectFile;
				}),
			projects.end());

		projects.insert(
			projects.begin(),
			RecentProjectEntry{
				project.name,
				project.engineVersion,
				projectFile,
			});

		if(projects.size() > maxCount) {
			projects.resize(maxCount);
		}
	}

} // namespace Calyx
