#include <CalyxEngine/CalyxEngine.h>

#include "GameApplication.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int){
	GameApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
