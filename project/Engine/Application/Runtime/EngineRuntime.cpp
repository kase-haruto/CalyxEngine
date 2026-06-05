#include <CalyxEngine/Application.h>
#include <CalyxEngine/Engine.h>
#include <CalyxEngine/Project.h>

#include <Engine/Application/Framework/CalyxFrameWork.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

#include <string>

namespace Calyx {

	namespace {

		std::string FirstCommandLineArgument(const char* commandLine) {
			if(!commandLine) return {};

			std::string input = commandLine;
			size_t		pos	  = input.find_first_not_of(" \t");
			if(pos == std::string::npos) return {};

			if(input[pos] == '"') {
				size_t end = input.find('"', pos + 1);
				if(end == std::string::npos) return input.substr(pos + 1);
				return input.substr(pos + 1, end - pos - 1);
			}

			size_t end = input.find_first_of(" \t", pos);
			return end == std::string::npos ? input.substr(pos) : input.substr(pos, end - pos);
		}

		void LoadProjectFromCommandLine(const char* commandLine, Application& application) {
			const std::string projectPath = FirstCommandLineArgument(commandLine);
			if(projectPath.empty()) return;

			ProjectInfo project;
			if(LoadProjectFile(projectPath, project)) {
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
