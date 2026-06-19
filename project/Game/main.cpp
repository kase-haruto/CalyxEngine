#include <CalyxEngine/CalyxEngine.h>
#include <Engine/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include <Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h>

#include "GameApplication.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int){
	CalyxEngine::RegisterGeneratedSceneObjects();
	CalyxEngine::RegisterGeneratedGameSceneObjects();

	GameApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
