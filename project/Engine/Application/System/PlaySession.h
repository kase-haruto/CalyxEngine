#pragma once

// engine
#include <Engine/Application/System/EngineMode.h>
#include <Engine/Scene/Context/SceneContext.h>
// externals
#include <externals/imgui/imgui.h>
// c++
#include <memory>

class PlaySession{
public:
	void Initialize(SceneContext* editorContext);
	void Enter();
	void Restart();
	void Exit(); 
	bool ExitRequested() const;
	void FinalizeExitCleanup();
	void TogglePause();
	void StepOnce();
	void Update(); 

	SceneContext* GetContext() const;
	void RebuildRuntimeFromEditor(SceneContext* newEditorCtx);

	void RenderToolbar();

	// SceneManager からの接続API
	void BindEditorContext(SceneContext* ctx);
	bool IsRuntime() const;

	uint64_t RuntimeGeneration() const { return runtimeGen_; }

private:
	SceneContext* editorContext_ = nullptr;
	std::unique_ptr<SceneContext> runtimeContext_;
	EngineMode mode_ = EngineMode::Editor;
	bool exitRequested_ = false;
	uint64_t runtimeGen_ = 0; 

	struct IconData{
		ImTextureID tex = nullptr;
		ImVec2 size = ImVec2(28, 28);
	};
	IconData iconPlay_;
	IconData iconPause_;
	IconData iconStep_;
	IconData iconStop_;
	IconData iconRestart_;
	void LoadIcons();
};