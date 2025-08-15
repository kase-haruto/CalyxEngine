#include "PlaySession.h"
#include <Engine/Scene/Serializer/SceneSerializer.h>
#include <Engine/Assets/Texture/TextureManager.h>

void PlaySession::Initialize(SceneContext* editorContext){
	editorContext_ = editorContext;
	editorContext_->SetRuntime(false);
	LoadIcons();

#ifndef NDEBUG
	// DebugビルドではEditorモード
	mode_ = EngineMode::Editor;
#else
	// Releaseビルドでは即実行
	Enter();
#endif
}

void PlaySession::LoadIcons(){
	auto& tm = *TextureManager::GetInstance();
	iconPlay_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/play.png").ptr;
	iconPause_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/pause.png").ptr;
	iconStep_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/step.png").ptr;
	iconRestart_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/restart.png").ptr;
	iconStop_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/stop.png").ptr;
}


void PlaySession::RenderToolbar(){
	ImGui::Begin("Play Toolbar",nullptr,
	             ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{4, 4});

	// --- decide which buttons to render ----------------------------------
	struct Btn
	{
		IconData* icon;
		const char* id;
		std::function<void()> onClick;
	};
	std::vector<Btn> buttons;

	switch (mode_){
	case EngineMode::Editor:
		buttons = {
					{&iconPlay_, "Play", [this]{ Enter(); }}
				};
		break;
	case EngineMode::Playing:
		buttons = {
					{&iconPause_, "Pause", [this]{ TogglePause(); }},
					{&iconRestart_, "Restart", [this]{ Restart(); }},
					{&iconStop_, "Stop", [this]{ Exit(); }}
				};
		break;
	case EngineMode::Paused:
		buttons = {
					{&iconPlay_, "Resume", [this]{ TogglePause(); }},
					{&iconStep_, "Step", [this]{ StepOnce(); }},
					{&iconRestart_, "Restart", [this]{ Restart(); }},
					{&iconStop_, "Stop", [this]{ Exit(); }}
				};
		break;
	case EngineMode::Step:
		buttons = {
					{
						&iconPause_, "Pause", [/*noop*/]{}
					},
					{&iconStop_, "Stop", [this]{ Exit(); }}
				};
		break;
	}

	// --- centering --------------------------------------------------------
	float spacing = ImGui::GetStyle().ItemSpacing.x;
	float totalW = 0.0f;
	for (size_t i = 0; i < buttons.size(); ++i) totalW += buttons[i].icon->size.x;
	totalW += spacing * static_cast<float>(buttons.size() - 1);
	float offset = (std::max)(0.0f,(ImGui::GetContentRegionAvail().x - totalW) * 0.5f);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

	// --- render loop ------------------------------------------------------
	for (size_t i = 0; i < buttons.size(); ++i){
		if (ImGui::ImageButton(buttons[i].icon->tex,buttons[i].icon->size))
			buttons[i].onClick();
		if (i + 1 < buttons.size()) ImGui::SameLine();
	}

	ImGui::PopStyleVar();
	ImGui::End();
}

void PlaySession::BindEditorContext(SceneContext* ctx){
	editorContext_ = ctx;
	if (editorContext_) editorContext_->SetRuntime(false);
}

bool PlaySession::IsRuntime() const{
	return mode_ == EngineMode::Playing||mode_==EngineMode::Step;
}

void PlaySession::Enter(){
	if (mode_ != EngineMode::Editor || !editorContext_) return;

	auto json = SceneSerializer::DumpJson(*editorContext_);
	runtimeContext_ = std::make_unique<SceneContext>();
	runtimeContext_->Initialize(false);
	runtimeContext_->MakeCurrent();
	SceneSerializer::LoadJson(*runtimeContext_, json);
	runtimeContext_->RunRuntimeBootstrap();
	runtimeContext_->SetRuntime(true);

	mode_ = EngineMode::Playing;
	++runtimeGen_;
}

void PlaySession::Restart(){
	if (mode_ == EngineMode::Playing || mode_ == EngineMode::Paused || mode_ == EngineMode::Step){
		auto json = SceneSerializer::DumpJson(*editorContext_);
		runtimeContext_ = std::make_unique<SceneContext>();
		runtimeContext_->Initialize(false);
		runtimeContext_->MakeCurrent();
		SceneSerializer::LoadJson(*runtimeContext_, json);
		runtimeContext_->RunRuntimeBootstrap();
		runtimeContext_->SetRuntime(true);
		mode_ = EngineMode::Playing;
		++runtimeGen_;
	}
}

void PlaySession::Exit(){
	if (mode_ == EngineMode::Editor) return;
	exitRequested_ = true;
	mode_ = EngineMode::Editor;
}

void PlaySession::RebuildRuntimeFromEditor(SceneContext* newEditorCtx){
	BindEditorContext(newEditorCtx);
	if (!IsRuntime()) return;

	auto json = SceneSerializer::DumpJson(*editorContext_);
	runtimeContext_ = std::make_unique<SceneContext>();
	runtimeContext_->Initialize(false);
	runtimeContext_->MakeCurrent();
	SceneSerializer::LoadJson(*runtimeContext_, json);
	runtimeContext_->RunRuntimeBootstrap();
	runtimeContext_->SetRuntime(true);
	++runtimeGen_;
}

bool PlaySession::ExitRequested() const { return exitRequested_; }

void PlaySession::FinalizeExitCleanup() {
	// Editor側をアクティブに戻す（MakeCurrentは保険）
	if (editorContext_) {
		editorContext_->MakeCurrent();
		editorContext_->SetRuntime(false);
	}
	// ここで runtime を安全に破棄
	runtimeContext_.reset();
	exitRequested_ = false;
	mode_ = EngineMode::Editor;
}

void PlaySession::TogglePause(){
	if (mode_ == EngineMode::Playing){ mode_ = EngineMode::Paused; }
	else if (mode_ == EngineMode::Paused){ mode_ = EngineMode::Playing; }
}

void PlaySession::StepOnce(){ if (mode_ == EngineMode::Paused){ mode_ = EngineMode::Step; } }

void PlaySession::Update(){
	if (mode_ == EngineMode::Step) mode_ = EngineMode::Paused;
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


