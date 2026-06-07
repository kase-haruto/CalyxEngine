#include "ProjectBrowser.h"

#include <Engine/Assets/Manager/AssetManager.h>

#include <externals/imgui/ImGuiFileDialog.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <random>
#include <span>
#include <sstream>
#include <vector>

namespace CalyxEditor {

	namespace {

		// 定義されているプロジェクトテンプレートの一覧
		const ProjectBrowser::TemplateInfo kTemplates[] = {
			{ProjectTemplateType::Blank, "Blank", "空のプロジェクト", "Source", ""},
			{ProjectTemplateType::Demo, "Demo", "デモプロジェクト", "Source", "Resources/Assets/Scenes/DemoScene.scene"},
		};

		// 新規プロジェクトの初期作成先のディレクトリパスを取得（ユーザーのドキュメントフォルダを指す）
		std::filesystem::path DefaultUserProjectDirectory() {
			char*  userProfile = nullptr;
			size_t length		= 0;
			// WindowsのUSERPROFILE環境変数からパスを取得
			if(_dupenv_s(&userProfile, &length, "USERPROFILE") == 0 && userProfile) {
				std::filesystem::path path = std::filesystem::path(userProfile) / "Documents" / "Calyx Projects";
				std::free(userProfile);
				return path;
			}
			// 取得に失敗した場合は相対パスのフォルダ名を返す
			return std::filesystem::path("Calyx Projects");
		}

		// std::array<char> 等の固定長バッファへの安全な文字列コピーを行うヘルパー
		bool CopyText(std::span<char> buffer, const std::string& text) {
			if(buffer.empty()) return false;
			// バッファ終端用のヌル文字スペースを除いた最大長でコピー
			const size_t length = (std::min)(buffer.size() - 1, text.size());
			std::copy_n(text.data(), length, buffer.data());
			buffer[length] = '\0'; // 確実にヌル終端する
			return length == text.size();
		}

		// 入力欄の未入力判定
		bool IsBlank(const char* text) {
			if(!text) return true;
			while(*text) {
				if(!std::isspace(static_cast<unsigned char>(*text))) return false;
				++text;
			}
			return true;
		}

		// Visual Studio用GUID（UUID v4に準拠）を生成するヘルパー関数
		std::string MakeGuid() {
			std::random_device randomDevice;
			std::mt19937 generator(randomDevice());
			std::uniform_int_distribution<unsigned int> dist(0, 15);
			std::uniform_int_distribution<unsigned int> variantDist(8, 11);

			// GUID形式: XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX のフォーマットで書き出し
			std::stringstream stream;
			stream << std::uppercase << std::hex;
			const int groups[] = {8, 4, 3};
			for(int group : groups) {
				for(int i = 0; i < group; ++i) {
					stream << dist(generator);
				}
				stream << "-";
			}
			stream << "4"; // バージョン4フラグ
			for(int i = 0; i < 3; ++i) {
				stream << dist(generator);
			}
			stream << "-";
			stream << variantDist(generator); // バリアントビット
			for(int i = 0; i < 3; ++i) {
				stream << dist(generator);
			}
			stream << "-";
			for(int i = 0; i < 12; ++i) {
				stream << dist(generator);
			}
			return stream.str();
		}

		// vcxproj等のXMLタグ属性へ書き込める安全なエスケープ文字列に変換
		std::string EscapeXml(const std::string& text) {
			std::string result;
			result.reserve(text.size());
			for(char c : text) {
				switch(c) {
				case '&': result += "&amp;"; break;
				case '<': result += "&lt;"; break;
				case '>': result += "&gt;"; break;
				case '"': result += "&quot;"; break;
				default: result += c; break;
				}
			}
			return result;
		}

		// ソリューションやプロジェクトファイルをディスクへ書き出すヘルパー
		bool WriteTextFile(const std::filesystem::path& path, const std::string& text) {
			std::error_code ec;
			// 保存先ディレクトリが存在しない場合は親階層を含め自動作成
			std::filesystem::create_directories(path.parent_path(), ec);
			if(ec) return false;

			std::ofstream file(path);
			if(!file) return false;
			file << text;
			return true;
		}

		// ディレクトリ以下のファイルを再帰的にまとめてコピー（フォルダ構造維持）
		bool CopyDirectoryTree(const std::filesystem::path& source, const std::filesystem::path& destination) {
			if(!std::filesystem::exists(source)) {
				return false;
			}

			std::error_code ec;
			std::filesystem::create_directories(destination, ec);
			if(ec) return false;

			// ソースディレクトリ以下のファイルを再帰的に巡回コピー
			for(const auto& entry : std::filesystem::recursive_directory_iterator(source, ec)) {
				if(ec) return false;

				// 相対パスを求めて出力先パスを構成
				const auto relativePath = std::filesystem::relative(entry.path(), source, ec);
				if(ec) return false;

				const auto outputPath = destination / relativePath;
				if(entry.is_directory()) {
					std::filesystem::create_directories(outputPath, ec);
					if(ec) return false;
					continue;
				}

				std::filesystem::create_directories(outputPath.parent_path(), ec);
				if(ec) return false;

				// ファイルを上書きモードでコピー
				std::filesystem::copy_file(entry.path(), outputPath, std::filesystem::copy_options::overwrite_existing, ec);
				if(ec) return false;
			}

			return true;
		}

		// 現在のアプリケーション実行位置から上に向かって探索し、配布済みエンジンルートを発見する
		std::filesystem::path FindInstalledEngineDirectory() {
			std::error_code ec;
			std::filesystem::path path = std::filesystem::weakly_canonical(std::filesystem::current_path(), ec);
			if(ec) {
				path = std::filesystem::current_path();
			}

			// 親ディレクトリへ順に遡り、配布物に含まれるResourcesとTemplatesの存在を確認
			while(!path.empty()) {
				if(std::filesystem::exists(path / "Resources") && std::filesystem::exists(path / "Templates")) {
					return path;
				}
				if(std::filesystem::exists(path / "project" / "Resources") && std::filesystem::exists(path / "project" / "Templates")) {
					return path / "project";
				}

				const auto parent = path.parent_path();
				if(parent == path) break;
				path = parent;
			}

			return {};
		}

		// 基準フォルダ(from)から対象ファイル(to)への相対パスを取得し、Visual Studio形式（ジェネリック表記スラッシュ）に変換
		std::string ToVisualStudioPath(const std::filesystem::path& from, const std::filesystem::path& to) {
			std::error_code ec;
			auto relative = std::filesystem::relative(to, from, ec);
			if(ec) {
				relative = to;
			}
			return relative.generic_string();
		}

		// Source以下のcppをvcxprojへ登録するために集める
		std::vector<std::filesystem::path> CollectSourceFiles(const Calyx::ProjectInfo& project) {
			std::vector<std::filesystem::path> sourceFiles;
			const auto sourceDirectory = Calyx::ResolveProjectPath(project, project.sourceDirectory);
			if(!std::filesystem::exists(sourceDirectory)) {
				return sourceFiles;
			}

			std::error_code ec;
			for(const auto& entry : std::filesystem::recursive_directory_iterator(sourceDirectory, ec)) {
				if(ec) break;
				if(entry.is_regular_file() && entry.path().extension() == ".cpp") {
					sourceFiles.push_back(entry.path());
				}
			}

			std::sort(sourceFiles.begin(), sourceFiles.end());
			return sourceFiles;
		}

		std::string MakeGameApplicationHeader() {
			std::stringstream stream;
			stream << "#pragma once\n\n";
			stream << "#include <CalyxEngine/Application.h>\n\n";
			stream << "class GameApplication : public Calyx::Application {\n";
			stream << "public:\n";
			stream << "\tvoid RegisterScenes(Calyx::SceneRegistry& registry) override;\n";
			stream << "\tbool ShouldRenderEngineUi() const override;\n";
			stream << "};\n";
			return stream.str();
		}

		std::string MakeGameApplicationSource(const ProjectBrowser::TemplateInfo& selectedTemplate) {
			std::stringstream stream;
			stream << "#include \"GameApplication.h\"\n\n";
			stream << "#include <CalyxEngine/SceneRegistry.h>\n";

			if(selectedTemplate.type == ProjectTemplateType::Demo) {
				stream << "#include <Demo/Scene/DemoScene/DemoScene.h>\n\n";
				stream << "void GameApplication::RegisterScenes(Calyx::SceneRegistry& registry) {\n";
				stream << "\tregistry.AddScene<DemoScene>(0);\n";
				stream << "\tregistry.SetStartupScene(0);\n";
				stream << "}\n\n";
			} else {
				stream << "#include <Engine/Scene/Base/BaseScene.h>\n\n";
				stream << "void GameApplication::RegisterScenes(Calyx::SceneRegistry& registry) {\n";
				stream << "\tregistry.AddScene<BaseScene>(0);\n";
				stream << "\tregistry.SetStartupScene(0);\n";
				stream << "}\n\n";
			}

			stream << "bool GameApplication::ShouldRenderEngineUi() const {\n";
			stream << "\treturn false;\n";
			stream << "}\n";
			return stream.str();
		}

		// 生成されたゲームが最初に持つエントリポイント
		std::string MakeGameMainSource(const ProjectBrowser::TemplateInfo& selectedTemplate) {
			(void)selectedTemplate;
			std::stringstream stream;
			stream << "#include <CalyxEngine/CalyxEngine.h>\n\n";
			stream << "#include \"GameApplication.h\"\n\n";

			stream << "int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int) {\n";
			stream << "\tGameApplication application;\n";
			stream << "\treturn Calyx::Run(hInstance, application, commandLine);\n";
			stream << "}\n";
			return stream.str();
		}

		// ゲームリポジトリ用の除外設定
		std::string MakeGameGitIgnore() {
			return
				"Generated/\n"
				".vs/\n"
				"*.user\n"
				"*.suo\n"
				"*.VC.db\n"
				"*.VC.opendb\n"
				"*.pdb\n"
				"*.ilk\n"
				"*.obj\n"
				"*.log\n";
		}

		// ゲームプロジェクト側の初期README
		std::string MakeGameReadme(const Calyx::ProjectInfo& project) {
			std::stringstream stream;
			stream << "# " << project.name << "\n\n";
			stream << "## Build\n\n";
			stream << "Set `CALYX_ENGINE_SDK_DIR` to the installed Calyx engine SDK directory before opening this solution.\n\n";
			stream << "PowerShell example:\n\n";
			stream << "```powershell\n";
			stream << "[Environment]::SetEnvironmentVariable(\"CALYX_ENGINE_SDK_DIR\", \"C:\\\\Calyx\\\\Engines\\\\1.2.0\\\\SDK\", \"User\")\n";
			stream << "```\n\n";
			stream << "After setting the variable, reopen Visual Studio and build this solution with `Develop|x64`.\n";
			return stream.str();
		}

		// ゲーム用vcxprojを生成
		std::string MakeGameVcxproj(
			const Calyx::ProjectInfo& project,
			const std::vector<std::filesystem::path>& sourceFiles,
			const std::string& projectGuid) {

			const std::string projectName = EscapeXml(project.name);
			const std::string sourceDir = project.sourceDirectory.generic_string();
			const std::string sdk = "$(CALYX_ENGINE_SDK_DIR)";

			std::stringstream stream;
			stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			stream << "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
			stream << "  <ItemGroup Label=\"ProjectConfigurations\">\n";
			stream << "    <ProjectConfiguration Include=\"Debug|x64\"><Configuration>Debug</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "    <ProjectConfiguration Include=\"Release|x64\"><Configuration>Release</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "    <ProjectConfiguration Include=\"Develop|x64\"><Configuration>Develop</Configuration><Platform>x64</Platform></ProjectConfiguration>\n";
			stream << "  </ItemGroup>\n";
			stream << "  <PropertyGroup Label=\"Globals\">\n";
			stream << "    <VCProjectVersion>17.0</VCProjectVersion>\n";
			stream << "    <Keyword>Win32Proj</Keyword>\n";
			stream << "    <ProjectGuid>{" << projectGuid << "}</ProjectGuid>\n";
			stream << "    <RootNamespace>" << projectName << "</RootNamespace>\n";
			stream << "    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>\n";
			stream << "    <ProjectName>" << projectName << "</ProjectName>\n";
			stream << "  </PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\" Label=\"Configuration\"><ConfigurationType>Application</ConfigurationType><UseDebugLibraries>true</UseDebugLibraries><PlatformToolset>v143</PlatformToolset><CharacterSet>Unicode</CharacterSet></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\" Label=\"Configuration\"><ConfigurationType>Application</ConfigurationType><UseDebugLibraries>false</UseDebugLibraries><PlatformToolset>v143</PlatformToolset><WholeProgramOptimization>true</WholeProgramOptimization><CharacterSet>Unicode</CharacterSet></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\" Label=\"Configuration\"><ConfigurationType>Application</ConfigurationType><UseDebugLibraries>false</UseDebugLibraries><PlatformToolset>v143</PlatformToolset><WholeProgramOptimization>true</WholeProgramOptimization><CharacterSet>Unicode</CharacterSet></PropertyGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n";
			stream << "  <ImportGroup Label=\"ExtensionSettings\" />\n";
			stream << "  <ImportGroup Label=\"Shared\" />\n";
			stream << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" /></ImportGroup>\n";
			stream << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" /></ImportGroup>\n";
			stream << "  <ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" /></ImportGroup>\n";
			stream << "  <PropertyGroup Label=\"UserMacros\" />\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir><LinkIncremental>true</LinkIncremental></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><IntDir>Generated\\Obj\\$(ProjectName)\\$(Configuration)\\</IntDir><OutDir>Generated\\Outputs\\$(Configuration)\\</OutDir></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Debug|x64'\"><LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\"><LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory></PropertyGroup>\n";
			stream << "  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Develop|x64'\"><LocalDebuggerWorkingDirectory>$(ProjectDir)</LocalDebuggerWorkingDirectory></PropertyGroup>\n";

			const char* configs[] = {"Debug", "Release", "Develop"};
			for(const char* config : configs) {
				const bool debug = std::string(config) == "Debug";
				stream << "  <ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='" << config << "|x64'\">\n";
				stream << "    <ClCompile>\n";
				stream << "      <PreprocessorDefinitions>" << (debug ? "_DEBUG" : (std::string(config) == "Develop" ? "DEVELOP" : "NDEBUG")) << ";_WINDOWS;%(PreprocessorDefinitions)</PreprocessorDefinitions>\n";
				stream << "      <ConformanceMode>true</ConformanceMode>\n";
				stream << "      <LanguageStandard>stdcpp20</LanguageStandard>\n";
				stream << "      <AdditionalOptions>/utf-8 %(AdditionalOptions)</AdditionalOptions>\n";
				stream << "      <MultiProcessorCompilation>true</MultiProcessorCompilation>\n";
				stream << "      <TreatWarningAsError>true</TreatWarningAsError>\n";
				stream << "      <AdditionalIncludeDirectories>$(ProjectDir);$(ProjectDir)" << sourceDir << ";" << sdk << "\\Include;" << sdk << "\\Include\\Engine\\Application;" << sdk << "\\ThirdParty;" << sdk << "\\ThirdParty\\DirectXTex;" << sdk << "\\ThirdParty\\assimp\\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>\n";
				stream << "      <RuntimeLibrary>" << (debug ? "MultiThreadedDebug" : "MultiThreaded") << "</RuntimeLibrary>\n";
				stream << "      <BasicRuntimeChecks>Default</BasicRuntimeChecks>\n";
				if(debug) {
					stream << "      <DebugInformationFormat>ProgramDatabase</DebugInformationFormat>\n";
				}
				stream << "    </ClCompile>\n";
				stream << "    <Link>\n";
				stream << "      <SubSystem>Windows</SubSystem>\n";
				if(!debug) {
					stream << "      <EnableCOMDATFolding>true</EnableCOMDATFolding>\n";
					stream << "      <OptimizeReferences>true</OptimizeReferences>\n";
				}
				stream << "      <GenerateDebugInformation>true</GenerateDebugInformation>\n";
				stream << "      <AdditionalDependencies>CalyxEngineLib.lib;DirectXTex.lib;" << (debug ? "assimp-vc143-mtd.lib" : "assimp-vc143-mt.lib") << ";%(AdditionalDependencies)</AdditionalDependencies>\n";
				stream << "      <AdditionalLibraryDirectories>$(CALYX_ENGINE_SDK_DIR)\\Lib\\$(Configuration);$(CALYX_ENGINE_SDK_DIR)\\Lib\\DirectXTex\\x64\\" << (debug ? "Debug" : "Release") << ";$(CALYX_ENGINE_SDK_DIR)\\Lib\\assimp\\" << (debug ? "Debug" : "Release") << ";%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>\n";
				stream << "      <AdditionalOptions>/IGNORE:4099 %(AdditionalOptions)</AdditionalOptions>\n";
				stream << "    </Link>\n";
				stream << "    <PostBuildEvent><Command>if exist \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxcompiler.dll\" copy \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxcompiler.dll\" \"$(TargetDir)dxcompiler.dll\"\n";
				stream << "if exist \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxil.dll\" copy \"$(WindowsSdkDir)bin\\$(TargetPlatformVersion)\\x64\\dxil.dll\" \"$(TargetDir)dxil.dll\"</Command></PostBuildEvent>\n";
				stream << "  </ItemDefinitionGroup>\n";
			}

			stream << "  <ItemGroup>\n";
			for(const auto& sourceFile : sourceFiles) {
				stream << "    <ClCompile Include=\"" << EscapeXml(ToVisualStudioPath(project.rootDirectory, sourceFile)) << "\" />\n";
			}
			stream << "  </ItemGroup>\n";
			stream << "  <Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n";
			stream << "  <Target Name=\"ValidateCalyxEngineSdkDir\" BeforeTargets=\"PrepareForBuild\">\n";
			stream << "    <Error Condition=\"'$(CALYX_ENGINE_SDK_DIR)'==''\" Text=\"CALYX_ENGINE_SDK_DIR is not set. Set it to the installed Calyx engine SDK directory and reopen Visual Studio.\" />\n";
			stream << "    <Error Condition=\"'$(CALYX_ENGINE_SDK_DIR)'!='' and !Exists('$(CALYX_ENGINE_SDK_DIR)\\Include\\CalyxEngine\\Application.h')\" Text=\"CALYX_ENGINE_SDK_DIR does not point to a Calyx SDK directory: $(CALYX_ENGINE_SDK_DIR)\" />\n";
			stream << "    <Error Condition=\"'$(CALYX_ENGINE_SDK_DIR)'!='' and !Exists('$(CALYX_ENGINE_SDK_DIR)\\Lib\\$(Configuration)\\CalyxEngineLib.lib')\" Text=\"CalyxEngineLib.lib was not found for $(Configuration). Update or reinstall the Calyx engine SDK: $(CALYX_ENGINE_SDK_DIR)\" />\n";
			stream << "  </Target>\n";
			stream << "  <ImportGroup Label=\"ExtensionTargets\" />\n";
			stream << "</Project>\n";
			return stream.str();
		}

		// Visual Studio上で見やすいフィルタを生成
		std::string MakeGameFilters(const Calyx::ProjectInfo& project, const std::vector<std::filesystem::path>& sourceFiles) {

			std::stringstream stream;
			stream << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
			stream << "<Project ToolsVersion=\"4.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n";
			stream << "  <ItemGroup>\n";
			stream << "    <Filter Include=\"Game\"><UniqueIdentifier>{" << MakeGuid() << "}</UniqueIdentifier></Filter>\n";
			stream << "  </ItemGroup>\n";
			stream << "  <ItemGroup>\n";
			for(const auto& sourceFile : sourceFiles) {
				stream << "    <ClCompile Include=\"" << EscapeXml(ToVisualStudioPath(project.rootDirectory, sourceFile)) << "\"><Filter>Game</Filter></ClCompile>\n";
			}
			stream << "  </ItemGroup>\n";
			stream << "</Project>\n";
			return stream.str();
		}

		// ゲーム用slnを生成
		std::string MakeGameSolution(const Calyx::ProjectInfo& project, const std::string& projectGuid, const std::string& solutionGuid) {
			const std::string projectName = project.name;
			const std::string projectFile = (project.name + ".vcxproj");

			std::stringstream stream;
			stream << "\nMicrosoft Visual Studio Solution File, Format Version 12.00\n";
			stream << "# Visual Studio Version 17\n";
			stream << "VisualStudioVersion = 17.5.33627.172\n";
			stream << "MinimumVisualStudioVersion = 10.0.40219.1\n";
			stream << "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"" << projectName << "\", \"" << projectFile << "\", \"{" << projectGuid << "}\"\n";
			stream << "EndProject\n";
			stream << "Global\n";
			stream << "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n";
			stream << "\t\tDebug|x64 = Debug|x64\n\t\tDevelop|x64 = Develop|x64\n\t\tRelease|x64 = Release|x64\n";
			stream << "\tEndGlobalSection\n";
			stream << "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n";
			stream << "\t\t{" << projectGuid << "}.Debug|x64.ActiveCfg = Debug|x64\n";
			stream << "\t\t{" << projectGuid << "}.Debug|x64.Build.0 = Debug|x64\n";
			stream << "\t\t{" << projectGuid << "}.Develop|x64.ActiveCfg = Develop|x64\n";
			stream << "\t\t{" << projectGuid << "}.Develop|x64.Build.0 = Develop|x64\n";
			stream << "\t\t{" << projectGuid << "}.Release|x64.ActiveCfg = Release|x64\n";
			stream << "\t\t{" << projectGuid << "}.Release|x64.Build.0 = Release|x64\n";
			stream << "\tEndGlobalSection\n";
			stream << "\tGlobalSection(SolutionProperties) = preSolution\n";
			stream << "\t\tHideSolutionNode = FALSE\n";
			stream << "\tEndGlobalSection\n";
			stream << "\tGlobalSection(ExtensibilityGlobals) = postSolution\n";
			stream << "\t\tSolutionGuid = {" << solutionGuid << "}\n";
			stream << "\tEndGlobalSection\n";
			stream << "EndGlobal\n";
			return stream.str();
		}

		// 新規ゲームプロジェクト用のソリューション、プロジェクトファイル、起動用GameMain.cppなどを一式作成する
		bool CreateGameWorkspace(const Calyx::ProjectInfo& project, const ProjectBrowser::TemplateInfo& selectedTemplate) {
			// 配布済みエンジンのインストールディレクトリを取得（Resourcesやテンプレートコピーのため）
			const auto engineDirectory = FindInstalledEngineDirectory();
			if(engineDirectory.empty()) {
				return false;
			}

			// ソリューションおよびvcxprojファイルで紐付ける新規の個別GUIDを発行
			const std::string projectGuid = MakeGuid();
			const std::string solutionGuid = MakeGuid();

			// ゲーム実行時にもエンジン共通の画像やシェーダーリソースを使うため、Resourcesディレクトリを丸ごとコピー
			if(!CopyDirectoryTree(engineDirectory / "Resources", project.rootDirectory / "Resources")) {
				return false;
			}
			// デモテンプレートの場合は、事前定義されたデモアセットやシーン一式（Templates/Demo）をコピー
			if(selectedTemplate.type == ProjectTemplateType::Demo) {
				const auto templateDirectory = engineDirectory / "Templates" / "Demo";
				if(!CopyDirectoryTree(templateDirectory, project.rootDirectory)) {
					return false;
				}
			}

			// アプリのエントリポイントとなる初期「GameMain.cpp」ファイルを生成してSource下に書き出し
			const auto sourceDirectory = Calyx::ResolveProjectPath(project, project.sourceDirectory);
			if(!WriteTextFile(sourceDirectory / "GameMain.cpp", MakeGameMainSource(selectedTemplate))) {
				return false;
			}
			if(!WriteTextFile(sourceDirectory / "GameApplication.h", MakeGameApplicationHeader())) {
				return false;
			}
			if(!WriteTextFile(sourceDirectory / "GameApplication.cpp", MakeGameApplicationSource(selectedTemplate))) {
				return false;
			}

			// フォルダ内からソースファイル一覧を収集
			const auto sourceFiles = CollectSourceFiles(project);

			// Visual Studioでビルド可能にするための各定義ファイルをテキストとして生成し書き出す
			if(!WriteTextFile(project.rootDirectory / (project.name + ".vcxproj"), MakeGameVcxproj(project, sourceFiles, projectGuid))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / (project.name + ".vcxproj.filters"), MakeGameFilters(project, sourceFiles))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / (project.name + ".sln"), MakeGameSolution(project, projectGuid, solutionGuid))) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / ".gitignore", MakeGameGitIgnore())) {
				return false;
			}
			if(!WriteTextFile(project.rootDirectory / "README.md", MakeGameReadme(project))) {
				return false;
			}

			return true;
		}

	} // namespace

	////////////////////////////////////////////////////////////////////////////////////////////
	//						初期化
	////////////////////////////////////////////////////////////////////////////////////////////
	ProjectBrowser::ProjectBrowser()
		: registryPath_(Calyx::DefaultProjectRegistryPath()) {
		// 表示レイアウトやサイズ用のパラメータをロードし、存在しなければ新規作成
		if(!param_.LoadParams()) {
			param_.SaveParams();
		}

		// UI入力バッファの初期化（デフォルトプロジェクト名とユーザーごとの既定フォルダパス）
		CopyText(newProjectName_, "NewProject");
		CopyText(newProjectDirectory_, DefaultUserProjectDirectory().string());
		// 最近使ったプロジェクトリストをレジストリからロード
		ReloadRecentProjects();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						描画
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::Draw(Calyx::ProjectInfo& outProject) {
		
		bool selected = false;

		// ビューポートサイズいっぱいのブラウザウインドウを固定配置で描画
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::Begin("Project Browser", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
		// 描画アイコンテクスチャをアセットシステムから必要に応じて確保
		LoadIcons();
		
		ImGui::TextUnformatted("Calyx Project Browser");
		ImGui::Separator();

		// 「プロジェクトを開く」ボタンによる既存 `.calyxproj` 選択ダイアログの表示
		if(ImGui::Button("Open Project", ImVec2(param_.openButtonSize_.x, param_.openButtonSize_.y))) {
			IGFD::FileDialogConfig config;
			ImGuiFileDialog::Instance()->OpenDialog("OpenCalyxProject", "Open Calyx Project", ".calyxproj", config);
		}
		ImGui::SameLine();
		// 「更新」ボタンによる最近使ったリストの再スキャン
		if(ImGui::Button("Refresh", ImVec2(param_.refreshButtonSize_.x, param_.refreshButtonSize_.y))) {
			ReloadRecentProjects();
		}

		// ステータスエラー等があれば上部にテキストで通知
		if(!statusMessage_.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("%s", statusMessage_.c_str());
		}

		ImGui::Spacing();
		ImGuiStyle& style = ImGui::GetStyle();

		// 新規プロジェクト作成の入力フォーム（下部フッター）の表示高さを事前計算
		const float footerHeight =
			ImGui::GetFrameHeightWithSpacing() * 2.0f +
			style.ItemSpacing.y * 2.0f +
			style.WindowPadding.y;

		// フッターエリアを除いたメイン描画領域
		if(ImGui::BeginChild(
			"ProjectBrowserMainArea",
			ImVec2(0.0f, -footerHeight),
			false,
			ImGuiWindowFlags_NoScrollbar)) {

			// 左右に「最近使ったプロジェクト / テンプレート一覧」と「選択中テンプレート詳細情報」を配置
			if(ImGui::BeginTable(
				"ProjectBrowserLayout",
				param_.tableColumnCount_,
				ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV,
				ImVec2(0.0f, 0.0f))) {

				ImGui::TableSetupColumn("Recent", ImGuiTableColumnFlags_WidthStretch, param_.recentColumnWeight_);
				ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, param_.detailsColumnWeight_);

				// 左ペイン：最近使った＆テンプレートカード一覧
				ImGui::TableNextColumn();
				DrawRecentProjects(outProject, selected);

				// 右ペイン：選択中のテンプレートの詳細情報表示
				ImGui::TableNextColumn();
				DrawTemplateDetails();

				ImGui::EndTable();
				}
			}
		ImGui::EndChild();

		ImGui::Separator();
		// 下部フッター：新規プロジェクト作成用UI
		DrawNewProject(outProject, selected);
		// 各ポップアップファイルダイアログの遅延・更新処理
		DrawOpenProjectDialog(outProject, selected);
		DrawLocationDialog();

		ImGui::End();
		return selected;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクトの再読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::ReloadRecentProjects() {
		if(!Calyx::LoadRecentProjects(registryPath_, recentProjects_)) {
			recentProjects_.clear();
			statusMessage_ = "Recent project list could not be loaded.";
			return;
		}
		statusMessage_.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクト一覧
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawRecentProjects(Calyx::ProjectInfo& outProject, bool& selected) {
		DrawTemplateCards();

		ImGui::Spacing();
		ImGui::TextUnformatted("Recent Projects");
		ImGui::Separator();

		if(recentProjects_.empty()) {
			ImGui::TextDisabled("No recent projects.");
		} else {
			ImGui::BeginChild("RecentProjectList", ImVec2(0.0f, 0.0f), false);

			const float cardWidth  = param_.cardSize_.x;
			const float spacing	= ImGui::GetStyle().ItemSpacing.x;
			const float availableWidth = ImGui::GetContentRegionAvail().x;
			int columns = static_cast<int>(availableWidth / (cardWidth + spacing));
			columns = (std::max)(1, columns);

			if(ImGui::BeginTable("RecentProjectCards", columns)) {
				for(size_t i = 0; i < recentProjects_.size(); ++i) {
					ImGui::TableNextColumn();
					ImGui::PushID(static_cast<int>(i));
					DrawRecentProjectCard(recentProjects_[i], outProject, selected);
					ImGui::PopID();
				}
				ImGui::EndTable();
			}

			ImGui::EndChild();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレート一覧
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateCards() {
		ImGui::TextUnformatted("Project Templates");
		ImGui::Separator();

		const float cardWidth  = param_.templateCardSize_.x;
		const float spacing = ImGui::GetStyle().ItemSpacing.x;
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		int columns = static_cast<int>(availableWidth / (cardWidth + spacing));
		columns = (std::max)(1, columns);

		if(ImGui::BeginTable("TemplateCards", columns)) {
			for(size_t i = 0; i < std::size(kTemplates); ++i) {
				ImGui::TableNextColumn();
				ImGui::PushID(static_cast<int>(i));
				DrawTemplateCard(kTemplates[i]);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレートカード
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateCard(const TemplateInfo& item) {
		const bool selected = selectedTemplate_ == item.type;
		const ImVec2 cardSize(param_.templateCardSize_.x, param_.templateCardSize_.y);
		const ImVec2 cardMin = ImGui::GetCursorScreenPos();

		if(ImGui::Selectable("##template-card", selected, 0, cardSize)) {
			selectedTemplate_ = item.type;
		}

		const ImVec2 cardMax(cardMin.x + cardSize.x, cardMin.y + cardSize.y);
		auto* drawList = ImGui::GetWindowDrawList();
		const ImU32 borderColor = selected
			? ImGui::GetColorU32(ImGuiCol_CheckMark)
			: ImGui::ColorConvertFloat4ToU32(ImVec4(
				  param_.cardBorderColor_.x,
				  param_.cardBorderColor_.y,
				  param_.cardBorderColor_.z,
				  param_.cardBorderColor_.w));
		drawList->AddRect(cardMin, cardMax, borderColor);

		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardPadding_));
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		} else {
			ImGui::Dummy(ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		}

		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardNameOffsetY_));
		ImGui::TextWrapped("%s", item.name);
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardVersionOffsetY_));
		ImGui::TextDisabled("Template");

		ImGui::SetCursorScreenPos(cardMax);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						最近使ったプロジェクトカード
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawRecentProjectCard(const Calyx::RecentProjectEntry& entry, Calyx::ProjectInfo& outProject, bool& selected) {
		const ImVec2 cardSize(param_.cardSize_.x, param_.cardSize_.y);
		const ImVec2 cardMin = ImGui::GetCursorScreenPos();

		if(ImGui::Selectable("##recent-card", false, 0, cardSize)) {
			selected = LoadProject(entry.projectFile, outProject);
		}

		const ImVec2 cardMax(cardMin.x + cardSize.x, cardMin.y + cardSize.y);
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRect(
			cardMin,
			cardMax,
			ImGui::ColorConvertFloat4ToU32(ImVec4(
				param_.cardBorderColor_.x,
				param_.cardBorderColor_.y,
				param_.cardBorderColor_.z,
				param_.cardBorderColor_.w)));

		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardPadding_));
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		} else {
			ImGui::Dummy(ImVec2(cardSize.x - param_.cardImageWidthOffset_, param_.cardImageHeight_));
		}

		const std::string label = entry.name.empty() ? entry.projectFile.stem().string() : entry.name;
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardNameOffsetY_));
		ImGui::TextWrapped("%s", label.c_str());
		ImGui::SetCursorScreenPos(ImVec2(cardMin.x + param_.cardPadding_, cardMin.y + param_.cardVersionOffsetY_));
		ImGui::TextDisabled("%s", entry.engineVersion.empty() ? "0.1.0" : entry.engineVersion.c_str());

		ImGui::SetCursorScreenPos(cardMax);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						テンプレート詳細
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawTemplateDetails() {
		const TemplateInfo& item = GetSelectedTemplate();

		ImGui::TextUnformatted("Template");
		ImGui::Separator();

		const float previewWidth = ImGui::GetContentRegionAvail().x;
		const float previewHeight = (std::min)(param_.templatePreviewMaxHeight_, previewWidth * param_.templatePreviewAspect_);
		if(genericIcon_) {
			ImGui::Image(genericIcon_, ImVec2(previewWidth, previewHeight));
		} else {
			ImGui::Dummy(ImVec2(previewWidth, previewHeight));
		}

		ImGui::Spacing();
		ImGui::TextUnformatted(item.name);
		ImGui::TextWrapped("%s", item.description);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextDisabled("Language");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("C++");

		ImGui::TextDisabled("Target");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted("Desktop");

		ImGui::TextDisabled("Startup Scene");
		ImGui::SameLine(param_.templateValueOffsetX_);
		ImGui::TextUnformatted(item.startupScene[0] == '\0' ? "None" : item.startupScene);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						新規プロジェクト作成欄
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawNewProject(Calyx::ProjectInfo& outProject, bool& selected) {
		ImGui::BeginGroup();

		const float labelWidth	= param_.labelWidth_;
		const float buttonWidth	= param_.buttonWidth_;
		const float browseSize	= param_.browseSize_;
		const float fullWidth	= ImGui::GetContentRegionAvail().x;
		const float inputWidth	= (std::max)(param_.minInputWidth_, fullWidth - labelWidth - browseSize - ImGui::GetStyle().ItemSpacing.x * param_.inputSpacingCount_);

		ImGui::TextUnformatted("Project Location");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth(inputWidth);
		ImGui::InputText("##ProjectDirectory", newProjectDirectory_.data(), newProjectDirectory_.size());
		ImGui::SameLine();
		if(folderIcon_) {
			if(ImGui::ImageButton("##BrowseProjectLocation", folderIcon_, ImVec2(param_.folderIconSize_, param_.folderIconSize_))) {
				IGFD::FileDialogConfig config;
				config.path = newProjectDirectory_.data();
				ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
			}
		} else if(ImGui::Button("...", ImVec2(param_.fallbackBrowseButtonSize_.x, param_.fallbackBrowseButtonSize_.y))) {
			IGFD::FileDialogConfig config;
			config.path = newProjectDirectory_.data();
			ImGuiFileDialog::Instance()->OpenDialog("SelectProjectDirectory", "Select Project Location", nullptr, config);
		}

		ImGui::TextUnformatted("Project Name");
		ImGui::SameLine(labelWidth);
		ImGui::SetNextItemWidth((std::max)(param_.minInputWidth_, fullWidth - labelWidth - buttonWidth - ImGui::GetStyle().ItemSpacing.x * param_.inputSpacingCount_));
		ImGui::InputText("##ProjectName", newProjectName_.data(), newProjectName_.size());

		ImGui::SameLine();
		if(ImGui::Button("Create", ImVec2(buttonWidth, param_.createButtonHeight_))) {
			selected = CreateProjectFromSelectedTemplate(outProject);
		}

		ImGui::EndGroup();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						既存プロジェクトを開くダイアログ
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawOpenProjectDialog(Calyx::ProjectInfo& outProject, bool& selected) {
		if(ImGuiFileDialog::Instance()->Display("OpenCalyxProject")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				selected = LoadProject(ImGuiFileDialog::Instance()->GetFilePathName(), outProject);
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						プロジェクト作成先フォルダの選択
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::DrawLocationDialog() {
		if(ImGuiFileDialog::Instance()->Display("SelectProjectDirectory")) {
			if(ImGuiFileDialog::Instance()->IsOk()) {
				CopyText(newProjectDirectory_, ImGuiFileDialog::Instance()->GetCurrentPath());
			}
			ImGuiFileDialog::Instance()->Close();
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						アイコン読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	void ProjectBrowser::LoadIcons() {
		if(genericIcon_ && folderIcon_) {
			return;
		}

		auto* assetManager = CalyxEngine::AssetManager::GetInstance();
		if(!assetManager || !assetManager->GetTextureManager()) {
			return;
		}

		auto* textureManager = assetManager->GetTextureManager();
		if(!genericIcon_) {
			genericIcon_ = reinterpret_cast<void*>(textureManager->LoadTexture("UI/Tool/AssetPanel/generic.png").ptr);
		}
		if(!folderIcon_) {
			folderIcon_ = reinterpret_cast<void*>(textureManager->LoadTexture("UI/Tool/AssetPanel/folder.png").ptr);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						プロジェクト読み込み
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::LoadProject(const std::filesystem::path& path, Calyx::ProjectInfo& outProject) {
		Calyx::ProjectInfo project;
		if(!Calyx::LoadProjectFile(path, project)) {
			statusMessage_ = "Project could not be opened.";
			return false;
		}

		outProject = std::move(project);
		statusMessage_.clear();
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						Blankプロジェクト作成
	////////////////////////////////////////////////////////////////////////////////////////////
	bool ProjectBrowser::CreateProjectFromSelectedTemplate(Calyx::ProjectInfo& outProject) {
		if(IsBlank(newProjectName_.data()) || IsBlank(newProjectDirectory_.data())) {
			statusMessage_ = "Project name and directory are required.";
			return false;
		}

		const TemplateInfo& selectedTemplate = GetSelectedTemplate();

		Calyx::ProjectInfo project;
		project.name			 = newProjectName_.data();
		project.engineVersion	 = "0.1.0";
		project.rootDirectory	 = std::filesystem::path(newProjectDirectory_.data()) / project.name;
		project.projectFile		 = project.rootDirectory / (project.name + ".calyxproj");
		project.assetDirectory	 = "Resources/Assets";
		project.sourceDirectory	 = selectedTemplate.sourceDirectory;
		project.startupScene		 = selectedTemplate.startupScene;
		project.templateName		 = selectedTemplate.name;

		if(!Calyx::CreateProject(project)) {
			statusMessage_ = "Project could not be created.";
			return false;
		}
		if(!CreateGameWorkspace(project, selectedTemplate)) {
			statusMessage_ = "Visual Studio project files could not be created.";
			return false;
		}

		return LoadProject(project.projectFile, outProject);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						選択中テンプレートの取得
	////////////////////////////////////////////////////////////////////////////////////////////
	const ProjectBrowser::TemplateInfo& ProjectBrowser::GetSelectedTemplate() const {
		for(const TemplateInfo& item : kTemplates) {
			if(item.type == selectedTemplate_) {
				return item;
			}
		}
		return kTemplates[0];
	}

} // namespace CalyxEditor
