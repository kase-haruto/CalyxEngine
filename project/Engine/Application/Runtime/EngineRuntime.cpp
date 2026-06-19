#include <CalyxEngine/Application.h>
#include <CalyxEngine/Engine.h>
#include <CalyxEngine/Project.h>

#include <Engine/Application/Framework/CalyxFrameWork.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

#include <string>
#include <vector>

namespace Calyx {

	namespace {

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
			if(args.empty()) return;

			ProjectInfo project;
			if(LoadProjectFile(args.front(), project)) {
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

		frameWork.Initialize(hInstance, &application);
		application.OnInitialize();
		frameWork.Run(&application);
		application.OnFinalize();
		frameWork.Finalize();

		return 0;
	}

} // namespace Calyx
