#pragma once
// engine
#include <Engine/Application/UI/EngineUI/DebugOverlay.h>
#include <Engine/Application/UI/EngineUI/EditorMenu.h>
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Application/UI/EngineUI/PerformanceOverlay.h>
#include <Engine/Application/UI/EngineUI/Viewport.h>
#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Application/UI/Panels/EditorPanel.h>
#include <Engine/Application/UI/Panels/HierarchyPanel.h>
#include <Engine/Application/UI/Panels/InspectorPanel.h>
#include <Engine/Application/UI/Panels/KeyframePanel.h>
#include <Engine/Application/UI/Panels/LivePPPanel.h>
#include <Engine/Application/UI/Panels/LogPanel.h>
#include <Engine/Application/UI/Panels/MaterialNodeEditorPanel.h>
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Application/UI/Panels/PostEffectNodeEditorPanel.h>
#include <Engine/Application/UI/Panels/SpriteAnimationEditorPanel.h>
#include <Engine/Application/UI/Panels/SplineEditorPanel.h>
#include <Engine/Editor/DebugCameraFocusController.h>
#include <Engine/Editor/EditorSelectionCoordinator.h>
#include <Engine/Editor/Extension/EditorToolRegistry.h>
#include <Engine/Editor/ImGuiLayoutSwitcher.h>
#include <Engine/Editor/SceneObjectEditor.h>
#include <externals/nlohmann/json.hpp>

// c++
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace EngineEdit {
	enum class EditorMode {
		Edit,
		Game
	};

	enum class EditToolMode {
		Object,
		Object2D,
		SpriteAnimation,
		Prefab,
		ParticleEffect,
		PostEffect,
		Material,
		Animation
	};
} // namespace EngineEdit

class BaseEditor;
class SceneContext;
class SceneObject;
class BaseCamera;
class WorldTransform;
struct CalyxEngine::Vector2;
struct CalyxEngine::Matrix4x4;
namespace CalyxEngine {
	class SceneManager;
}

namespace CalyxEngine {

	class PlaySession;
	class SceneSwitchOverlay;
	class SceneSettingsWindow;
	class ImGuiLayoutSwitcher;
	class ParticlePreviewSession;
	class PrefabEditSession;
	class ViewportSelectionController;
	class IEditToolState;

	/*-----------------------------------------------------------------------------------------
	 * LevelEditor
	 * - レベルエディタクラス
	 * - シーンオブジェクトの選択・配置・削除、ビューポート操作、シーン保存を統合管理
	 *---------------------------------------------------------------------------------------*/
	class LevelEditor {
	public:
		LevelEditor();
		void Initialize();
		void Update();
		void Render();
		void RenderMenu();
		void RenderSettingsWindow();
		void RenderRuntimeFullscreenViewport(const ImTextureID& tex);
		void ClearSelection();
		~LevelEditor();
		CalyxEngine::SceneManager* GetSceneManager() const { return sceneManager_; }
		void					  SetSceneManager(CalyxEngine::SceneManager* sceneManager);

		// 編集対象 ----------------------------------------------------------------
		void SetSelectedEditor(BaseEditor* editor);
		/// SceneObject の選択（shared_ptr で受けて内部では weak_ptr で保持）
		void SetSelectedObject(const std::shared_ptr<SceneObject>& sp);
		void ToggleSelectedObject(const std::shared_ptr<SceneObject>& sp);
		void SetSelectedObjects(const std::vector<std::shared_ptr<SceneObject>>& objects);
		bool IsSelectedObject(const SceneObject* object) const;
		std::shared_ptr<SceneObject> GetPrimarySelectedObject() const;
		std::vector<std::shared_ptr<SceneObject>> GetSelectedObjects() const;
		void SaveSceneFromEditorTool() { SaveScene(); }
		bool IsPlaying() const;

		/// シーンへのオブジェクト追加（Prefab / PlaceTool などから呼ばれる）
		void CreateObject(const std::shared_ptr<SceneObject>& obj);
		/// シーンからオブジェクト削除（階層パネルなどから呼ばれる）
		void DeleteObject(const std::shared_ptr<SceneObject>& sp);
		/// Terminal等のエディタ操作からシーンファイルを開く。
		bool OpenSceneFromEditor(const std::filesystem::path& path);
		void DeleteSelectedObjects();
		std::vector<WorldTransform*> DuplicateSelectedObjects();

		// ビューポート関連 --------------------------------------------------------
		void RenderViewport(ViewportType type, const ImTextureID& tex);
		void SetCameraForViewport(BaseCamera* mainCamera, BaseCamera* debugCamera);

		// パネル取得 --------------------------------------------------------------
		HierarchyPanel* GetHierarchyPanel() const { return hierarchy_.get(); }
		EditorPanel*	GetEditorPanel() const { return editor_.get(); }
		PlaceToolPanel* GetPlaceToolPanel() const { return placeToolPanel_.get(); }

		EngineEdit::EditorMode GetMode() const { return mode_; }
		void				   SetPlaySession(PlaySession* session);
		bool				   ShouldRenderRuntimeFullscreen() const;
		bool				   ShouldHideEditorUiInGameMode() const;
		bool IsDebugViewportVisible()const;
		CalyxEditor::EditorToolRegistry& GetEditorToolRegistry() { return editorToolRegistry_; }

	private:
		// シーン管理 --------------------------------------------------------------
		void SaveScene();
		/**
		 * @brief 指定された保存先へ空のシーンファイルを作成し、Editorの編集対象として開く
		 * @param destination 作成する.sceneファイルのパス
		 * @return ファイル作成、保存、読込、切り替えに成功した場合はtrue
		 * @note 読込に失敗した場合、現在編集中のシーンはSceneManager側で維持される
		 */
		bool CreateNewScene(const std::filesystem::path& destination);

		/**
		 * @brief 指定されたシーンファイルをEditorの編集対象として開く
		 * @param path 読み込む.sceneファイルのパス
		 * @return 読込とEditor UI更新に成功した場合はtrue
		 * @note 読込に失敗した場合、現在編集中のシーンは維持される
		 */
		bool OpenScene(const std::filesystem::path& path);
		void NotifySceneContextChanged();
		void DrawSceneSelector();

		// モード切り替え ----------------------------------------------------------
		void EnterGameMode();
		void ExitGameMode();
		void ToggleMode();
		void DrawEditModeCombo();
		void ApplyEditToolMode(EngineEdit::EditToolMode mode, bool applyLayout);
		void SaveActiveModeSelection();
		void RestoreModeSelection(EngineEdit::EditToolMode mode);
		SceneContext* ResolveModeContext(EngineEdit::EditToolMode mode) const;
		SceneContext* ResolvePreviewContext(EngineEdit::EditToolMode mode) const;
		void ActivateModeContext(EngineEdit::EditToolMode mode);
		const char* GetEditToolModeName(EngineEdit::EditToolMode mode) const;
		std::string GetEditToolModeLayoutPath(EngineEdit::EditToolMode mode) const;
		std::string GetEditToolModeLoadLayoutPath(EngineEdit::EditToolMode mode) const;
		void EnsureParticlePreviewContext();
		void UpdateParticlePreviewContext(float dt);
		void EnsurePrefabEditContext();
		void NewPrefabEditContext(const std::string& rootTypeName = {});
		void OpenPrefabForEdit(const std::string& path);
		void SavePrefabEdit();
		void SavePrefabEditAs(const std::string& path);
		void ApplyPrefabOverridesFromInstance(const std::shared_ptr<SceneObject>& object);
		void UpdatePrefabEditContext(float dt);

		void TogglePanel(IEngineUI* p) {
			if(p) p->SetShow(!p->IsShow());
		}

	private:
		// 管理UI
		std::unique_ptr<HierarchyPanel>		 hierarchy_;
		std::unique_ptr<EditorPanel>		 editor_;
		std::unique_ptr<InspectorPanel>		 inspector_;
		std::unique_ptr<KeyframePanel>		 keyframePanel_;
		std::unique_ptr<SceneObjectEditor>	 sceneEditor_;
		std::unique_ptr<PlaceToolPanel>		 placeToolPanel_;
		std::unique_ptr<SplineEditorPanel>	 splineEditor_;
		std::unique_ptr<AssetPanel>			 assetPanel_;
		std::unique_ptr<MaterialNodeEditorPanel> materialNodeEditorPanel_;
		std::unique_ptr<PostEffectNodeEditorPanel> postEffectNodeEditorPanel_;
		std::unique_ptr<SpriteAnimationEditorPanel> spriteAnimationEditorPanel_;
		std::unique_ptr<LivePPPanel>		 livePPPanel_;
		std::unique_ptr<LogPanel>			 logPanel_;
		std::unique_ptr<SceneSwitchOverlay>	 sceneSwitchOverlay_;
		std::unique_ptr<SceneSettingsWindow> sceneSettingsWindow_;
		std::unique_ptr<ImGuiLayoutSwitcher> layoutSwitcher_;
		std::unique_ptr<DebugCameraFocusController> debugCameraFocus_;
		PlaySession*						 pPlaySesseion_ = nullptr;
		CalyxEngine::SceneManager*			 sceneManager_	= nullptr;

		// メニュー
		std::unique_ptr<EditorMenu> menu_; //< エディターメニュー
		EngineEdit::EditorMode		mode_ = EngineEdit::EditorMode::Edit;
		EngineEdit::EditToolMode	editToolMode_ = EngineEdit::EditToolMode::Object;
		const IEditToolState* editToolState_ = nullptr; //< 所有権を持たない現在の静的編集モードState

		// ビューポート
		std::unique_ptr<Viewport>			mainViewport_;		 //< メインビューポート
		std::unique_ptr<Viewport>			debugViewport_;		 //< デバッグビューポート
		std::unique_ptr<Viewport>			pickingViewport_;	 //< ピッキングビューポート
		std::unique_ptr<Viewport>			pickingDepthViewport_; //< ピッキング深度ビューポート
		int									startupDebugViewportFocusFrames_ = 3;
		std::unique_ptr<PerformanceOverlay> performanceOverlay_; //< パフォーマンスオーバーレイ
		std::unique_ptr<DebugOverlay>		debugOverlay_;		 //< デバッグオーバーレイ
		std::unique_ptr<ParticlePreviewSession> particlePreview_;
		std::unique_ptr<PrefabEditSession> prefabEdit_;
		std::unique_ptr<ViewportSelectionController> viewportSelection_;

		// 状態
		bool		  lastPlaying_	  = false;
		SceneContext* prevCtx_		  = nullptr;
		EditorSelectionCoordinator selection_;
		std::unordered_map<EngineEdit::EditToolMode, EditorSelectionCoordinator::Snapshot> modeSelections_;
		EngineEdit::EditToolMode activeSelectionMode_ = EngineEdit::EditToolMode::Object;
		nlohmann::json			   livePPSnapshot_;
		// シーン保存のポップアップ表示
		float		  sceneSavedPopupTimer_ = 0.0f;
		std::string   sceneSavedPopupPath_;

		// Editors メニューに並べるパネル群
		std::vector<IEngineUI*> editorPanels_;
		CalyxEditor::EditorToolRegistry editorToolRegistry_;
	};

} // namespace CalyxEngine
