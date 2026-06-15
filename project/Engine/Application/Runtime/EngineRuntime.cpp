#include <CalyxEngine/Application.h>
#include <CalyxEngine/Engine.h>
#include <CalyxEngine/Project.h>

#include <Engine/Application/Framework/CalyxFrameWork.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

#include <string>
#include <vector>

namespace Calyx {

	namespace {

		std::vector<std::string> ParseCommandLineArguments(const char* commandLine) {
			std::vector<std::string> args;
			if(!commandLine) return args;

			std::string input = commandLine;
			size_t pos = 0;
			while(pos < input.size()) {
				// 空白を読み飛ばして、次の引数の開始位置へ進める。
				pos = input.find_first_not_of(" \t", pos);
				if(pos == std::string::npos) break;

				// ダブルクォートで囲まれたパスは、空白を含む 1 つの引数として扱う。
				if(input[pos] == '"') {
					const size_t end = input.find('"', pos + 1);
					if(end == std::string::npos) {
						args.push_back(input.substr(pos + 1));
						break;
					}
					args.push_back(input.substr(pos + 1, end - pos - 1));
					pos = end + 1;
					continue;
				}

				// クォートされていない通常引数は、次の空白までを 1 引数として扱う。
				const size_t end = input.find_first_of(" \t", pos);
				args.push_back(end == std::string::npos ? input.substr(pos) : input.substr(pos, end - pos));
				if(end == std::string::npos) break;
				pos = end + 1;
			}

			return args;
		}

		void LoadProjectFromCommandLine(const char* commandLine, Application& application) {
			const auto args = ParseCommandLineArguments(commandLine);
			if(args.empty()) return;

			// 先頭の通常引数は .calyxproj のパスとして扱う。
			// それ以降のフラグは起動時だけ使う情報で、プロジェクトファイルには保存しない。
			const std::string& projectPath = args.front();
			if(projectPath.empty()) return;

			ProjectInfo project;
			if(LoadProjectFile(projectPath, project)) {
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
