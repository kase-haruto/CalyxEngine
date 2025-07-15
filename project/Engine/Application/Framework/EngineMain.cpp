#define ENGINE_EXPORTS
#include "EngineMain.h"
#include <Engine/Application/Framework/EngineController.h>

// c++
#include <memory>

static std::unique_ptr<EngineController> engine;

void Engine_Initialize(HINSTANCE hInstance){
	engine = std::make_unique<EngineController>();
	engine->Initialize(hInstance);
}

bool Engine_Update(){
	// ここで false を返せば終了
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
