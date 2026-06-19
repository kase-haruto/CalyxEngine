#include <CalyxEngine/Project.h>

#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <utility>

namespace Calyx {

	namespace {

		//////////////////////////////////////////////////////////////////////////////
		// パスを可能な限り正規化する
		// weakly_canonical が失敗した場合は、文字列上の正規化のみ行う
		//////////////////////////////////////////////////////////////////////////////
		std::filesystem::path NormalizePath(const std::filesystem::path& path) {
			std::error_code ec;
			auto normalized = std::filesystem::weakly_canonical(path, ec);
			return ec ? path.lexically_normal() : normalized;
		}

		///////////////////////////////////////////////////////////////////////////////
		// json からパス文字列を読み取る
		// 指定キーが存在しない、または文字列でない場合は fallback を返す
		///////////////////////////////////////////////////////////////////////////////
		std::filesystem::path ReadPath(
			const nlohmann::json& root,
			const char*			  key,
			const std::filesystem::path& fallback) {

			if(!root.contains(key) || !root.at(key).is_string()) {
				return fallback;
			}
			return root.at(key).get<std::string>();
		}

		////////////////////////////////////////////////////////////////////////////////
		// パスを json に保存しやすい形式へ変換する
		// generic_string にすることで、区切り文字を '/' に統一する
		/////////////////////////////////////////////////////////////////////////////////
		std::string WritePath(const std::filesystem::path& path) {
			return path.generic_string();
		}

		//////////////////////////////////////////////////////////////////////////////////
		// プロジェクトルートからの相対パスへ変換する
		// すでに相対パスの場合は、そのまま正規化して返す
		///////////////////////////////////////////////////////////////////////////////////
		std::filesystem::path RelativeProjectPath(const ProjectInfo& project, const std::filesystem::path& path) {
			if(path.empty()) return {};
			if(!path.is_absolute()) return path.lexically_normal();

			std::error_code ec;
			auto relative = std::filesystem::relative(path, project.rootDirectory, ec);
			return ec ? path.lexically_normal() : relative.lexically_normal();
		}

	} // namespace

	///////////////////////////////////////////////////////////////////////////////////////////////
	// .calyxproj などのプロジェクトファイルを読み込み、ProjectInfo に変換する
	///////////////////////////////////////////////////////////////////////////////////////////////
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

		// プロジェクトファイルの位置を基準に、プロジェクトルートを決定する
		const auto projectFile = NormalizePath(path);
		const auto projectRoot = projectFile.parent_path();

		ProjectInfo project;

		// json に存在しない項目は、デフォルト値を使用する
		project.name			= root.value("name", projectRoot.filename().string());
		project.engineVersion	= root.value("engineVersion", std::string{});
		project.projectFile		= projectFile;
		project.rootDirectory	= projectRoot;
		project.assetDirectory	= ReadPath(root, "assetDirectory", "Resources/Assets");
		project.sourceDirectory	= ReadPath(root, "sourceDirectory", "Game");
		project.startupScene		= ReadPath(root, "startupScene", std::filesystem::path{});
		project.gameModule		= ReadPath(root, "gameModule", std::filesystem::path{});

		// 構成ごとの DLL 出力先を読み込む。
		// Debug/Develop/Release で生成される DLL の場所が違うため、Editor は起動構成に合わせてここから読み分ける。
		if(root.contains("gameModules") && root.at("gameModules").is_object()) {
			const auto& modules = root.at("gameModules");
			project.gameModuleDebug	 = ReadPath(modules, "Debug", project.gameModule);
			project.gameModuleDevelop = ReadPath(modules, "Develop", project.gameModule);
			project.gameModuleRelease = ReadPath(modules, "Release", project.gameModule);
		}
		project.templateName		= root.value("template", std::string{"Blank"});

		outProject = std::move(project);
		return true;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////
	// ProjectInfo の内容をプロジェクトファイルとして保存する
	//////////////////////////////////////////////////////////////////////////////////////////////////
	bool SaveProjectFile(const ProjectInfo& project) {
		if(project.projectFile.empty()) {
			return false;
		}

		// 保存先のディレクトリが存在しない場合は作成する
		std::error_code ec;
		const auto projectFileParent = project.projectFile.parent_path();
		if(!projectFileParent.empty()) {
			std::filesystem::create_directories(projectFileParent, ec);
		}
		if(ec) {
			return false;
		}

		nlohmann::json root;

		// プロジェクト情報を json に書き込む
		// ディレクトリやシーンのパスは、プロジェクトルートからの相対パスで保存する
		root["name"]			 = project.name;
		root["engineVersion"]	 = project.engineVersion;
		root["assetDirectory"]	 = WritePath(RelativeProjectPath(project, project.assetDirectory));
		root["sourceDirectory"]	 = WritePath(RelativeProjectPath(project, project.sourceDirectory));
		root["startupScene"]		 = WritePath(RelativeProjectPath(project, project.startupScene));
		root["gameModule"]		 = WritePath(RelativeProjectPath(project, project.gameModule));
		root["gameModules"]		 = nlohmann::json::object();
		// 構成ごとの DLL パスを保存する。
		// Visual Studio から Debug/Develop/Release を切り替えたとき、同じ .calyxproj から正しい DLL をロードするための情報。
		root["gameModules"]["Debug"]		= WritePath(RelativeProjectPath(project, project.gameModuleDebug));
		root["gameModules"]["Develop"]	= WritePath(RelativeProjectPath(project, project.gameModuleDevelop));
		root["gameModules"]["Release"]	= WritePath(RelativeProjectPath(project, project.gameModuleRelease));
		root["template"]			 = project.templateName.empty() ? "Blank" : project.templateName;

		std::ofstream file(project.projectFile);
		if(!file) {
			return false;
		}

		// インデント付きで保存し、人が読める形式にする
		file << root.dump(2);
		return true;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////
	// 新規プロジェクトに必要なディレクトリを作成し、プロジェクトファイルを保存する
	//////////////////////////////////////////////////////////////////////////////////////////////////////
	bool CreateProject(const ProjectInfo& project) {
		if(project.projectFile.empty() || project.rootDirectory.empty()) {
			return false;
		}

		std::error_code ec;

		// プロジェクトルートを作成する
		std::filesystem::create_directories(project.rootDirectory, ec);
		if(ec) return false;

		// アセット用ディレクトリを作成する
		std::filesystem::create_directories(ResolveProjectPath(project, project.assetDirectory), ec);
		if(ec) return false;

		// ソースコード用ディレクトリを作成する
		std::filesystem::create_directories(ResolveProjectPath(project, project.sourceDirectory), ec);
		if(ec) return false;

		// 最後にプロジェクトファイルを保存する
		return SaveProjectFile(project);
	}

	// プロジェクトルートを基準に、相対パスを絶対パスへ解決する
	// すでに絶対パスの場合はそのまま返す
	std::filesystem::path ResolveProjectPath(const ProjectInfo& project, const std::filesystem::path& path) {
		if(path.empty() || path.is_absolute()) {
			return path;
		}
		return NormalizePath(project.rootDirectory / path);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 最近開いたプロジェクト一覧を保存するデフォルトパスを取得する
	// Windows では LOCALAPPDATA/CalyxEngine/projects.json を使用する
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	std::filesystem::path DefaultProjectRegistryPath() {
		char*  localAppData = nullptr;
		size_t length		 = 0;

		if(_dupenv_s(&localAppData, &length, "LOCALAPPDATA") == 0 && localAppData) {
			std::filesystem::path path = std::filesystem::path(localAppData) / "CalyxEngine" / "projects.json";
			std::free(localAppData);
			return path;
		}

		// LOCALAPPDATA が取得できない場合のフォールバック
		return std::filesystem::path("CalyxEngine") / "projects.json";
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 最近開いたプロジェクト一覧を json から読み込む
	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool LoadRecentProjects(const std::filesystem::path& path, std::vector<RecentProjectEntry>& outProjects) {
		outProjects.clear();
		bool removedMissingProjects = false;

		// ファイルが存在しない場合は、空の一覧として扱う
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

		// projects が配列でなければ不正な形式として扱う
		if(!root.contains("projects") || !root.at("projects").is_array()) {
			return false;
		}

		// 有効な項目だけを RecentProjectEntry に変換する
		for(const auto& item : root.at("projects")) {
			if(!item.is_object() || !item.contains("projectFile") || !item.at("projectFile").is_string()) {
				continue;
			}

			RecentProjectEntry entry;
			entry.name			 = item.value("name", std::string{});
			entry.engineVersion	 = item.value("engineVersion", std::string{});
			entry.projectFile	 = item.at("projectFile").get<std::string>();

			// Recent は各ユーザー環境のローカル履歴なので、削除済みプロジェクトはここで捨てる。
			// 存在しない .calyxproj を残すと Project Browser に開けないカードが出続ける。
			if(!std::filesystem::exists(entry.projectFile)) {
				removedMissingProjects = true;
				continue;
			}

			outProjects.push_back(std::move(entry));
		}

		// 削除済みプロジェクトを除外した場合は、次回起動時にも消えた状態になるよう保存し直す。
		if(removedMissingProjects) {
			SaveRecentProjects(path, outProjects);
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 最近開いたプロジェクト一覧を json として保存する
	////////////////////////////////////////////////////////////////////////////////////////////////////////////
	bool SaveRecentProjects(const std::filesystem::path& path, const std::vector<RecentProjectEntry>& projects) {
		std::error_code ec;

		// 保存先ディレクトリが存在しない場合は作成する
		const auto registryParent = path.parent_path();
		if(!registryParent.empty()) {
			std::filesystem::create_directories(registryParent, ec);
		}
		if(ec) {
			return false;
		}

		nlohmann::json root;
		root["projects"] = nlohmann::json::array();

		// RecentProjectEntry を json 配列に変換する
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

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 最近開いたプロジェクト一覧に、指定プロジェクトを追加する
	// 既に同じプロジェクトが存在する場合は削除し、先頭へ移動する
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	void AddRecentProject(std::vector<RecentProjectEntry>& projects, const ProjectInfo& project, size_t maxCount) {
		const auto projectFile = NormalizePath(project.projectFile);

		// 同じプロジェクトファイルを指す履歴を削除する
		projects.erase(
			std::remove_if(
				projects.begin(),
				projects.end(),
				[&projectFile](const RecentProjectEntry& entry) {
					return NormalizePath(entry.projectFile) == projectFile;
				}),
			projects.end());

		// 最新のプロジェクトとして先頭に追加する
		projects.insert(
			projects.begin(),
			RecentProjectEntry{
				project.name,
				project.engineVersion,
				projectFile,
			});

		// 履歴数が上限を超えた場合は、古いものを削除する
		if(projects.size() > maxCount) {
			projects.resize(maxCount);
		}
	}

} // namespace Calyx
