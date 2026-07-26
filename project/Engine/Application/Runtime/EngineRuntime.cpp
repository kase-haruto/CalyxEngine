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
#include <vector>

namespace Calyx {

	void SetWindowTitle(const char* title) {
		const HWND window = CalyxEngine::CalyxCore::GetHWND();
		if(!window) return;

		const std::wstring wideTitle = ConvertString(title ? title : "");
		::SetWindowTextW(window, wideTitle.c_str());
	}

	namespace {

		std::string DefaultLaunchConfiguration() {
#if defined(_DEBUG)
			return "Debug";
#elif defined(DEVELOP)
			return "Develop";
#else
			return "Release";
#endif
		}

		std::filesystem::path FindAdjacentProjectFile() {
			std::wstring executablePath(MAX_PATH, L'\0');
			const DWORD pathLength = ::GetModuleFileNameW(
				nullptr,
				executablePath.data(),
				static_cast<DWORD>(executablePath.size()));
			if(pathLength == 0 || pathLength >= executablePath.size()) return {};
			executablePath.resize(pathLength);

			const auto executableDirectory = std::filesystem::path(executablePath).parent_path();

			std::filesystem::path projectFile;
			std::error_code error;
			for(const auto& entry : std::filesystem::directory_iterator(executableDirectory, error)) {
				if(error) return {};
				if(!entry.is_regular_file(error) || error) continue;
				if(entry.path().extension() != ".calyxproj") continue;

				if(!projectFile.empty()) return {};
				projectFile = entry.path();
			}
			return projectFile;
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

		void LoadProjectFromCommandLine(const char* commandLine, Application& application) {
			const auto args = SplitCommandLineArguments(commandLine);
			const auto projectFile = args.empty()
				? FindAdjacentProjectFile()
				: std::filesystem::path(args.front());
			if(projectFile.empty()) return;

			if(args.empty()) {
				std::error_code error;
				std::filesystem::current_path(projectFile.parent_path(), error);
				if(error) return;
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
				application.OnProjectLoaded(project);
			}
		}

	} // namespace

	int Run(HINSTANCE hInstance, Application& application) {
		return Run(hInstance, application, nullptr);
	}

	int Run(HINSTANCE hInstance, Application& application, const char* commandLine) {
		LeakChecker leakChecker_;
		CalyxEngine::CalyxFrameWork frameWork;

		LoadProjectFromCommandLine(commandLine, application);

		try {
			frameWork.Initialize(hInstance, &application);
			application.OnInitialize();
			frameWork.Run(&application);
			application.OnFinalize();
			frameWork.Finalize();
		} catch(const std::exception& exception) {
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
