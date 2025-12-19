#define ENGINE_EXPORTS
#include "EngineMain.h"
#include <Engine/Application/Framework/CalyxFrameWork.h>

// c++
#include <memory>

static std::unique_ptr<CalyxEngine::CalyxFrameWork> engine;

void Engine_Initialize(HINSTANCE hInstance){
	engine = std::make_unique<CalyxEngine::CalyxFrameWork>();
	engine->Initialize(hInstance);
}

bool Engine_Update(){
	return engine->Update();
}

void Engine_Render(){
	engine->Render();
}

void Engine_Finalize(){
	if (engine){
		engine->Finalize();
		engine.reset();
	}
}
