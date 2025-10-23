#include <Engine/Application/Framework/EngineController.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int){
	LeakChecker leakChecker_;
	EngineController EngineController;

	EngineController.Initialize(hInstance);
	EngineController.Run();
	EngineController.Finalize();

	return 0;
}