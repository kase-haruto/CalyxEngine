#include "PlaySession.h"
#include <Engine/Scene/Serializer/SceneSerializer.h>

void PlaySession::Initialize(SceneContext* editorContext){
	editorContext_ = editorContext;
	editorContext_->SetRuntime(false);
}


void PlaySession::Enter(){
	if (mode_ != EngineMode::Editor || !editorContext_) return;

	auto json = SceneSerializer::DumpJson(*editorContext_);
	runtimeContext_ = std::make_unique<SceneContext>();
	runtimeContext_->Initialize(false);
	SceneSerializer::LoadJson(*runtimeContext_, json);
	runtimeContext_->SetRuntime(true);

	mode_ = EngineMode::Playing;
}

void PlaySession::Exit(){
	if (mode_ == EngineMode::Editor) return;

	runtimeContext_.reset();
	editorContext_->SetRuntime(false);
	mode_ = EngineMode::Editor;
}

void PlaySession::TogglePause(){
	if (mode_ == EngineMode::Playing){
		mode_ = EngineMode::Paused;
	} else if (mode_ == EngineMode::Paused){
		mode_ = EngineMode::Playing;
	}
}

void PlaySession::StepOnce(){
	if (mode_ == EngineMode::Paused){
		mode_ = EngineMode::Step;
	}
}

void PlaySession::Restart(){
	if (mode_ == EngineMode::Playing || mode_ == EngineMode::Paused || mode_ == EngineMode::Step){
		auto json = SceneSerializer::DumpJson(*editorContext_);
		runtimeContext_ = std::make_unique<SceneContext>();
		runtimeContext_->Initialize(false);
		SceneSerializer::LoadJson(*runtimeContext_, json);
		runtimeContext_->SetRuntime(true);
		mode_ = EngineMode::Playing;
	}
}

void PlaySession::Update(float dt){
	SceneContext* active = GetContext();
	if (active) active->MakeCurrent();

	// ───────────────────────────────────────────────
	// Editor / Pause でも必須処理を回し、Play/Step だけゲームロジックも実行
	bool runtimePass = (mode_ == EngineMode::Playing || mode_ == EngineMode::Step);
	active->Update(dt,runtimePass);

	if (mode_ == EngineMode::Step){
		mode_ = EngineMode::Paused;
	}
}

SceneContext* PlaySession::GetContext() const{
	switch (mode_){
		case EngineMode::Playing:
		case EngineMode::Paused:
		case EngineMode::Step:
			return runtimeContext_.get();
		case EngineMode::Editor:
		default:
			return editorContext_;
	}
}
