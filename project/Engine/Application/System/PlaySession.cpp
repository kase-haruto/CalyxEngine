#include "PlaySession.h"
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Scene/Serializer/SceneSerializer.h>

namespace CalyxEditor {

	void PlaySession::Initialize(SceneContext* editorContext) {
		editorContext_ = editorContext;
		editorContext_->SetRuntime(false);
		LoadIcons();

#if defined(_DEBUG) || defined(DEVELOP)
		// DebugビルドではEditorモード
		mode_ = EngineMode::Editor;
#else
		// Releaseビルドでは即実行
		Enter();
#endif
	}

	void PlaySession::LoadIcons() {
		auto& tm		 = *TextureManager::GetInstance();
		iconPlay_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/play.dds").ptr;
		iconPause_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/pause.dds").ptr;
		iconStep_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/step.dds").ptr;
		iconRestart_.tex = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/restart.dds").ptr;
		iconStop_.tex	 = (ImTextureID)tm.LoadTexture("UI/Tool/ToolBar/stop.dds").ptr;
	}

	void PlaySession::RenderToolbar() {
		ImGui::Begin("Play Toolbar", nullptr,
					 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4, 4});

		// ボタン定義（常に5個すべて並べる）
		struct Btn {
			IconData*			  icon;
			const char*			  id;
			bool				  enabled;
			std::function<void()> onClick;
		};
		std::vector<Btn> buttons;

		// モードに応じて有効/無効のみ切り替える
		switch(mode_) {
		case EngineMode::Editor:
			buttons = {
				{&iconPlay_, "Play", true, [this] { Enter(); }},
				{&iconPause_, "Pause", false, [] {}},
				{&iconStep_, "Step", false, [] {}},
				{&iconRestart_, "Restart", false, [] {}},
				{&iconStop_, "Stop", false, [] {}},
			};
			break;
		case EngineMode::Playing:
			buttons = {
				{&iconPlay_, "Play", false, [] {}}, // 無効
				{&iconPause_, "Pause", true, [this] { TogglePause(); }},
				{&iconStep_, "Step", false, [] {}}, // 無効（ポーズ時のみ有効）
				{&iconRestart_, "Restart", true, [this] { Restart(); }},
				{&iconStop_, "Stop", true, [this] { Exit(); }},
			};
			break;
		case EngineMode::Paused:
			buttons = {
				{&iconPlay_, "Resume", true, [this] { TogglePause(); }}, // 再生（ポーズ解除）
				{&iconPause_, "Pause", false, [] {}},					 // すでにポーズ中
				{&iconStep_, "Step", true, [this] { StepOnce(); }},
				{&iconRestart_, "Restart", true, [this] { Restart(); }},
				{&iconStop_, "Stop", true, [this] { Exit(); }},
			};
			break;
		case EngineMode::Step:
			// Stepフレームは次のUpdateでPausedに戻るので、操作はStop/Restartのみ有効にしておく
			buttons = {
				{&iconPlay_, "Play", false, [] {}},
				{&iconPause_, "Pause", false, [] {}},
				{&iconStep_, "Step", false, [] {}},
				{&iconRestart_, "Restart", true, [this] { Restart(); }},
				{&iconStop_, "Stop", true, [this] { Exit(); }},
			};
			break;
		}

		// センタリング
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalW  = 0.0f;
		for(const auto& b : buttons) totalW += b.icon->size.x;
		totalW += spacing * static_cast<float>(buttons.size() - 1);
		float offset = (std::max)(0.0f, (ImGui::GetContentRegionAvail().x - totalW) * 0.5f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

		// 描画ヘルパー：無効時は半透明 + Disable状態で押下不可に
		auto drawButton = [&](const Btn& b) {
			if(!b.enabled) ImGui::BeginDisabled(true);
			// 無効時はアイコンを薄く
			ImVec4 tint	   = b.enabled ? ImVec4(1, 1, 1, 1) : ImVec4(1, 1, 1, 0.4f);
			bool   clicked = ImGui::ImageButton(b.icon->tex, b.icon->size, ImVec2(0, 0), ImVec2(1, 1), 0,
												ImVec4(0, 0, 0, 0), tint);
			if(!b.enabled) ImGui::EndDisabled();
			if(clicked && b.enabled && b.onClick) b.onClick();

			// ツールチップ（任意）
			if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(b.id);
				ImGui::EndTooltip();
			}
		};

		// ボタン列描画
		for(size_t i = 0; i < buttons.size(); ++i) {
			drawButton(buttons[i]);
			if(i + 1 < buttons.size()) ImGui::SameLine();
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}
	void PlaySession::BindEditorContext(SceneContext* ctx) {
		editorContext_ = ctx;
		if(editorContext_) editorContext_->SetRuntime(false);
	}

	bool PlaySession::IsRuntime() const {
		return mode_ == EngineMode::Playing || mode_ == EngineMode::Step;
	}

	void PlaySession::Enter() {
		if(mode_ != EngineMode::Editor || !editorContext_) return;

		auto json		= SceneSerializer::DumpJson(*editorContext_);
		runtimeContext_ = std::make_unique<SceneContext>();
		runtimeContext_->Initialize(false);
		runtimeContext_->MakeCurrent();
		SceneSerializer::LoadJson(*runtimeContext_, json);
		runtimeContext_->SetRuntime(true);

		mode_ = EngineMode::Playing;
		++runtimeGen_;
	}

	void PlaySession::Restart() {
		if(mode_ == EngineMode::Playing || mode_ == EngineMode::Paused || mode_ == EngineMode::Step) {
			auto json		= SceneSerializer::DumpJson(*editorContext_);
			runtimeContext_ = std::make_unique<SceneContext>();
			runtimeContext_->Initialize(false);
			runtimeContext_->MakeCurrent();
			SceneSerializer::LoadJson(*runtimeContext_, json);
			runtimeContext_->SetRuntime(true);
			mode_ = EngineMode::Playing;
			++runtimeGen_;
		}
	}

	void PlaySession::Exit() {
		if(mode_ == EngineMode::Editor) return;
		exitRequested_ = true;
		mode_		   = EngineMode::Editor;
	}

	void PlaySession::RebuildRuntimeFromEditor(SceneContext* newEditorCtx) {
		BindEditorContext(newEditorCtx);
		if(!IsRuntime()) return;

		auto json		= SceneSerializer::DumpJson(*editorContext_);
		runtimeContext_ = std::make_unique<SceneContext>();
		runtimeContext_->Initialize(false);
		runtimeContext_->MakeCurrent();
		SceneSerializer::LoadJson(*runtimeContext_, json);
		runtimeContext_->SetRuntime(true);
		++runtimeGen_;
	}

	bool PlaySession::ExitRequested() const { return exitRequested_; }

	void PlaySession::FinalizeExitCleanup() {
		// Editor側をアクティブに戻す（MakeCurrentは保険）
		if(editorContext_) {
			editorContext_->MakeCurrent();
			editorContext_->SetRuntime(false);
		}
		// ここで runtime を安全に破棄
		runtimeContext_.reset();
		exitRequested_ = false;
		mode_		   = EngineMode::Editor;
	}

	void PlaySession::TogglePause() {
		if(mode_ == EngineMode::Playing) {
			mode_ = EngineMode::Paused;
		} else if(mode_ == EngineMode::Paused) {
			mode_ = EngineMode::Playing;
		}
	}

	void PlaySession::StepOnce() {
		if(mode_ == EngineMode::Paused) {
			mode_ = EngineMode::Step;
		}
	}

	void PlaySession::Update() {
		if(mode_ == EngineMode::Step) mode_ = EngineMode::Paused;
	}

	SceneContext* PlaySession::GetContext() const {
		switch(mode_) {
		case EngineMode::Playing:
		case EngineMode::Paused:
		case EngineMode::Step:
			return runtimeContext_.get();
		case EngineMode::Editor:
		default:
			return editorContext_;
		}
	}

} // namespace CalyxEditor