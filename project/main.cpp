#include <Engine/Application/Framework/CalyxFrameWork.h>
#include <Engine/Foundation/Utility/LeakChecker/LeakChecker.h>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int){
	LeakChecker leakChecker_;
	CalyxEngine::CalyxFrameWork CalyxFrameWork;

	CalyxFrameWork.Initialize(hInstance);
	CalyxFrameWork.Run();
	CalyxFrameWork.Finalize();

	return 0;
}