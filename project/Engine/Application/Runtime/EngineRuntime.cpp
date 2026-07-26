#include <CalyxEngine/Application.h>
#include <CalyxEngine/Engine.h>
#include <CalyxEngine/Project.h>

#include <Engine/Application/Framework/CalyxFrameWork.h>
#include <Engine/Application/System/CalyxCore.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Foundation/Utility/Converter/ConvertString.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

#include <filesystem>
#include <string>
#include <filesystem>
#include <optional>
#include <vector>

namespace Calyx {

	void SetWindowTitle(const char* title) {
		const HWND window = CalyxEngine::CalyxCore::GetHWND();
		if(!window) return;

		const std::wstring wideTitle = ConvertString(title ? title : "");
		::SetWindowTextW(window, wideTitle.c_str());
	}

	namespace {

		std::optional<std::filesystem::path> FindSingleProjectFile(
			const std::filesystem::path& directory) {
			// 存在しない探索候補は例外にせず無視し、次の候補ディレクトリを試せるようにする。
			std::error_code ec;
			if(directory.empty() || !std::filesystem::is_directory(directory, ec)) {
				return std::nullopt;
			}

			std::optional<std::filesystem::path> result;
			for(std::filesystem::directory_iterator it(directory, ec), end; !ec && it != end; it.increment(ec)) {
				if(!it->is_regular_file(ec) || it->path().extension() != ".calyxproj") {
					continue;
				}
				if(result) {
					// 複数プロジェクトがある場所は自動選択せず、誤ったゲームを起動しない。
					return std::nullopt;
				}
				result = it->path();
			}
			return result;
		}

		std::optional<std::filesystem::path> DiscoverProjectFile() {
			std::vector<std::filesystem::path> searchDirectories;

			// IDE実行と配布物実行の両方を支えるため、作業ディレクトリと実行ファイル周辺を探索する。
			std::error_code ec;
			searchDirectories.push_back(std::filesystem::current_path(ec));

			wchar_t executablePath[MAX_PATH]{};
			if(::GetModuleFileNameW(nullptr, executablePath, MAX_PATH) > 0) {
				auto directory = std::filesystem::path(executablePath).parent_path();
				// Launcherやビルド出力から上位のprojectフォルダへ到達できる範囲に探索深度を制限する。
				for(size_t depth = 0; depth < 5 && !directory.empty(); ++depth) {
					searchDirectories.push_back(directory);
					searchDirectories.push_back(directory / "project");
					directory = directory.parent_path();
				}
			}

			for(const auto& directory : searchDirectories) {
				if(auto projectFile = FindSingleProjectFile(directory)) {
					return projectFile;
				}
			}
			return std::nullopt;
		}

		std::vector<std::string> SplitCommandLineArguments(const char* commandLine) {
			std::vector<std::string> args;
			if(!commandLine) return {};

			std::string input = commandLine;
			size_t		pos	  = 0;
			while(pos < input.size()) {
				pos = input.find_first_not_of(" \t", pos);
				if(pos == std::string::npos) break;

				// 空白を含むパスを受け取れるように、引用符で囲まれた引数は 1 つの文字列として扱う。
				if(input[pos] == '"') {
					size_t end = input.find('"', pos + 1);
					if(end == std::string::npos) {
						args.push_back(input.substr(pos + 1));
						break;
					}
					args.push_back(input.substr(pos + 1, end - pos - 1));
					pos = end + 1;
					continue;
				}

				size_t end = input.find_first_of(" \t", pos);
				args.push_back(end == std::string::npos ? input.substr(pos) : input.substr(pos, end - pos));
				if(end == std::string::npos) break;
				pos = end + 1;
			}

			return args;
		}

		bool LoadProjectFromCommandLine(const char* commandLine, Application& application) {
			auto args = SplitCommandLineArguments(commandLine);
			if(args.empty()) {
				// 明示引数がない開発時だけ自動探索し、曖昧な場合は起動を中止する。
				if(auto projectFile = DiscoverProjectFile()) {
					args.push_back(projectFile->string());
				} else {
					::MessageBoxW(
						nullptr,
						L"起動する .calyxproj を特定できませんでした。\n"
						L"プロジェクトファイルを引数に指定して起動してください。",
						L"CalyxGame",
						MB_OK | MB_ICONERROR);
					return false;
				}
			}

			ProjectInfo project;
			if(LoadProjectFile(projectFile, project)) {
				project.launchConfiguration = DefaultLaunchConfiguration();
				// Visual Studio/Launcher から渡された構成名を保持する。
				// Editor はこの値を見て Debug/Develop/Release のどのゲーム DLL をロードするか決める。
				for(size_t i = 1; i + 1 < args.size(); ++i) {
					if(args[i] == "--config") {
						project.launchConfiguration = args[i + 1];
						++i;
					}
				}
				SetCurrentProject(project);
				// Engine初期化より前に通知し、Applicationがプロジェクト依存設定を準備できるようにする。
				application.OnProjectLoaded(project);
				return true;
			}
			return false;
		}

	} // namespace

	int Run(HINSTANCE hInstance, Application& application) {
		return Run(hInstance, application, nullptr);
	}

	int Run(HINSTANCE hInstance, Application& application, const char* commandLine) {
		// LeakCheckerをFrameworkより先に生成し、Engineリソース解放後まで監視を継続する。
		LeakChecker leakChecker_;
		CalyxEngine::CalyxFrameWork frameWork;

		// プロジェクトが確定しない状態ではRendererやゲームDLLを初期化しない。
		if(!LoadProjectFromCommandLine(commandLine, application)) {
			return -1;
		}

		try {
			// 初期化と終了処理を同じスコープへまとめ、通常終了時のライフタイム順序を明示する。
			frameWork.Initialize(hInstance, &application);
			application.OnInitialize();
			frameWork.Run(&application);
			application.OnFinalize();
			frameWork.Finalize();
		} catch(const std::exception& exception) {
			// 起動失敗を境界で捕捉し、例外をWinMainの外へ伝播させずログへ残す。
			CalyxEngine::EngineLogger::GetInstance().Add(
				CalyxEngine::LogLevel::Error,
				CalyxEngine::LogCategory::Engine,
				std::string("Engine startup failed: ") + exception.what(),
				"EngineRuntime");
			return -1;
		}

		return 0;
	}

} // namespace Calyx
