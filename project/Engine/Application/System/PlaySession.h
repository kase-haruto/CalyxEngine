#pragma once

#include <Engine/Application/System/EngineMode.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <memory>

class PlaySession{
public:
	void Initialize(SceneContext* editorContext);

	// --- Play Control ---
	void Enter();
	void Exit();
	void TogglePause();
	void StepOnce();
	void Restart();

	// --- Update ---
	void Update(float dt);

	// --- Accessors ---
	SceneContext* GetContext() const;
	EngineMode GetMode() const{ return mode_; }

private:
	SceneContext* editorContext_ = nullptr;
	std::unique_ptr<SceneContext> runtimeContext_;
	EngineMode mode_ = EngineMode::Editor;
	bool stepRequested_ = false;
};