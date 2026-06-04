#include <CalyxEngine/Application.h>
#include <CalyxEngine/Engine.h>

#include <Engine/Application/Framework/CalyxFrameWork.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

namespace Calyx {

	int Run(HINSTANCE hInstance, Application& application) {
		LeakChecker leakChecker_;
		CalyxEngine::CalyxFrameWork frameWork;

		frameWork.Initialize(hInstance, &application);
		application.OnInitialize();
		frameWork.Run(&application);
		application.OnFinalize();
		frameWork.Finalize();

		return 0;
	}

} // namespace Calyx
