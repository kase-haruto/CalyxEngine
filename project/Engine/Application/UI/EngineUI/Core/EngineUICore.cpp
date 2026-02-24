////////////////////////////////////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////////////////////////////////////
#include <Engine/Application/System/Environment.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Application/UI/EngineUI/DebugTextManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// uiPanel
#include <Engine/Application/UI/Panels/ConsolePanel.h>
#include <Engine/Application/UI/Panels/EditorPanel.h>
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Editor/SceneSwitchOverlay.h>

// lib
#include <externals/imgui/ImGuizmo.h>
#include <externals/imgui/imgui.h>

namespace CalyxEditor {
	////////////////////////////////////////////////////////////////////////////////////////////
	//						初期化
	////////////////////////////////////////////////////////////////////////////////////////////
	void EngineUICore::Initialize() {
#if defined(_DEBUG) || defined(DEVELOP)
		panelController_ = std::make_unique<PanelController>();
		panelController_->Initialize();

		levelEditor_ = std::make_unique<LevelEditor>();
		levelEditor_->Initialize();
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						更新
	////////////////////////////////////////////////////////////////////////////////////////////
	void EngineUICore::Update() {
		DebugTextManager::Clear();
		levelEditor_->Update();
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						レンダリング
	////////////////////////////////////////////////////////////////////////////////////////////
	void EngineUICore::Render() {
#if defined(_DEBUG) || defined(DEVELOP)

		levelEditor_->RenderMenu();

		// === Gameモード中はUIなど表示しない ===
		if(levelEditor_->GetMode() == EngineEdit::EditorMode::Game) {
			return;
		}

		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

		levelEditor_->RenderViewport(ViewportType::VIEWPORT_MAIN, reinterpret_cast<ImTextureID>(mainViewportTextureID_));
		levelEditor_->RenderViewport(ViewportType::VIEWPORT_DEBUG, reinterpret_cast<ImTextureID>(debugViewportTextureID_));
		levelEditor_->RenderViewport(ViewportType::VIEWPORT_PICKING, reinterpret_cast<ImTextureID>(pickingViewportTextureID_));

		if(levelEditor_) {
			levelEditor_->Render();
		}

		// すべてのパネルをレンダリング
		panelController_->RenderPanels();

#endif // _DEBUG
	}

	HierarchyPanel* EngineUICore::GetHierarchyPanel() const {
		return levelEditor_->GetHierarchyPanel();
	}

	EditorPanel* EngineUICore::GetEditorPanel() const {
		return levelEditor_->GetEditorPanel();
	}

	PlaceToolPanel* EngineUICore::GetPlaceToolPanel() const {
		return levelEditor_->GetPlaceToolPanel();
	}

	void EngineUICore::SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera) {
		levelEditor_->SetCameraForViewport(mainCamera, debugCamera);
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						パネル追加
	////////////////////////////////////////////////////////////////////////////////////////////
	void EngineUICore::AddPanel([[maybe_unused]] std::unique_ptr<IEngineUI> panel) {
#if defined(_DEBUG) || defined(DEVELOP)
		const std::string& name = panel->GetPanelName(); // 先に名前を取り出す
		panelController_->RegisterPanel(name, std::move(panel));
#endif // _DEBUG
	}

	////////////////////////////////////////////////////////////////////////////////////////////
	//						メインビューポート用のテクスチャを設定
	////////////////////////////////////////////////////////////////////////////////////////////
	void EngineUICore::SetMainViewportTexture(UINT64 textureID) {
		if(mainViewportTextureID_) {
			return;
		}
		mainViewportTextureID_ = textureID;
	}

	void EngineUICore::SetDebugViewportTexture(UINT64 textureID) {
		if(debugViewportTextureID_) {
			return;
		}
		debugViewportTextureID_ = textureID;
	}

	void EngineUICore::SetPickingViewportTexture(UINT64 textureID) {
		if(pickingViewportTextureID_) {
			return;
		}
		pickingViewportTextureID_ = textureID;
	}
} // namespace CalyxEditor