#pragma once

#include <Engine/Application/System/EngineMode.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <memory>

class PlaySession{
public:
	void Initialize(SceneContext* editorContext);
	void Enter();
	void Restart();
	void Exit();
	void TogglePause();
	void StepOnce();
	void Update(float dt);
	SceneContext* GetContext() const;

	void RenderToolbar(); 

private:
	SceneContext* editorContext_ = nullptr;
	std::unique_ptr<SceneContext> runtimeContext_;
	EngineMode mode_ = EngineMode::Editor;

	struct IconData{
		ImTextureID tex = nullptr;
		ImVec2 size = ImVec2(28, 28);
	};
	IconData iconPlay_;
	IconData iconPause_;
	IconData iconStep_;
	IconData iconStop_;
	IconData iconRestart_;
	// [New]
	void LoadIcons();
};