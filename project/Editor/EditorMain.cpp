#include "ProjectBrowser.h"

#include <CalyxEngine/CalyxEngine.h>

#include <Demo/Scene/DemoScene/DemoScene.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Application/UI/EngineUI/Core/EngineUICore.h>
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include <Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h>
#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/System/SceneManager.h>

#include <filesystem>
#include <memory>
#include <vector>
#include <Windows.h>

namespace {

	class GameModule {
	public:
		using CreateApplication = Calyx::Application* (*)();
		using DestroyApplication = void (*)(Calyx::Application*);

		~GameModule() {
			Unload();
		}

		bool Load(const std::filesystem::path& modulePath) {
			Unload();
			if(modulePath.empty() || !std::filesystem::exists(modulePath)) {
				return false;
			}

			// ゲーム DLL を Editor プロセスに読み込む。
			// この時点で Visual Studio は DLL の PDB を認識できるため、ゲーム側のブレークポイントが有効になる。
			handle_ = ::LoadLibraryW(modulePath.wstring().c_str());
			if(!handle_) {
				return false;
			}

			create_ = reinterpret_cast<CreateApplication>(::GetProcAddress(handle_, "CreateCalyxApplication"));
			destroy_ = reinterpret_cast<DestroyApplication>(::GetProcAddress(handle_, "DestroyCalyxApplication"));
			if(!create_ || !destroy_) {
				Unload();
				return false;
			}

			return true;
		}

		Calyx::Application* Create() const {
			return create_ ? create_() : nullptr;
		}

		void Destroy(Calyx::Application* application) const {
			if(destroy_ && application) {
				destroy_(application);
			}
		}

	private:
		void Unload() {
			if(handle_) {
				::FreeLibrary(handle_);
				handle_ = nullptr;
			}
			create_ = nullptr;
			destroy_ = nullptr;
		}

		HMODULE handle_ = nullptr;
		CreateApplication create_ = nullptr;
		DestroyApplication destroy_ = nullptr;
	};

	std::filesystem::path SelectGameModulePath(const Calyx::ProjectInfo& project) {
		// Project Browser から直接開いた場合は構成名が渡らないため、エディタ向けの Develop DLL を既定にする。
		const std::string config = project.launchConfiguration.empty() ? "Develop" : project.launchConfiguration;
		if(config == "Debug" && !project.gameModuleDebug.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleDebug);
		if(config == "Develop" && !project.gameModuleDevelop.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleDevelop);
		if(config == "Release" && !project.gameModuleRelease.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleRelease);
		return Calyx::ResolveProjectPath(project, project.gameModule);
	}

} // namespace

class EditorApplication : public Calyx::Application {
public:
	~EditorApplication() override {
		FinalizeGameApplication();
	}

	void OnProjectLoaded(const Calyx::ProjectInfo& project) override {
		FinalizeGameApplication();
		project_ = project;
		hasProject_ = true;
		Calyx::SetCurrentProject(project_);
		AssetDatabase::GetInstance()->Initialize(Calyx::GetAssetRoot());

		std::vector<Calyx::RecentProjectEntry> recentProjects;
		const auto							  registryPath = Calyx::DefaultProjectRegistryPath();
		Calyx::LoadRecentProjects(registryPath, recentProjects);
		Calyx::AddRecentProject(recentProjects, project_);
		Calyx::SaveRecentProjects(registryPath, recentProjects);

		LoadGameApplication();
		if(gameApplication_) {
			gameApplication_->OnProjectLoaded(project_);
		}
		RefreshPlaceToolPanel();
		RegisterGameScenesIfReady();
		ApplyProjectTemplateScene();
	}

	void RegisterScenes(Calyx::SceneRegistry& registry) override {
		registry.AddScene<BaseScene>(kBlankSceneId);
		registry.AddScene<DemoScene>(kDemoSceneId);
		registry.SetStartupScene(kBlankSceneId);

		if(gameApplication_) {
			gameApplication_->RegisterScenes(registry);
			gameScenesRegistered_ = true;
		}
	}

	void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) override {
		sceneManager_ = &sceneManager;
		if(gameApplication_) {
			gameApplication_->OnSceneManagerReady(sceneManager);
		}
		RegisterGameScenesIfReady();
		ApplyProjectTemplateScene();
	}

	void OnEngineUiReady(CalyxEngine::EngineUICore& engineUi) override {
		engineUi_ = &engineUi;
		RefreshPlaceToolPanel();
	}

	void OnUpdate() override {
		if(hasProject_) {
			if(gameApplication_) {
				gameApplication_->OnUpdate();
			}
			return;
		}

		Calyx::ProjectInfo selectedProject;
		if(projectBrowser_.Draw(selectedProject)) {
			OnProjectLoaded(selectedProject);
		}
	}

	void OnRender() override {
		if(gameApplication_) {
			gameApplication_->OnRender();
		}
	}

	void OnFinalize() override {
		FinalizeGameApplication();
	}

	bool ShouldRenderEngineUi() const override {
		return hasProject_;
	}

private:
	void ApplyProjectTemplateScene() {
		if(!sceneManager_ || !hasProject_) {
			return;
		}

		if(gameApplication_ && gameScenesRegistered_) {
			return;
		}

		if(project_.templateName == "Demo") {
			sceneManager_->SetCurrent(kDemoSceneId);
			return;
		}

		sceneManager_->SetCurrent(kBlankSceneId);
	}

	void LoadGameApplication() {
		const auto modulePath = SelectGameModulePath(project_);
		if(!gameModule_.Load(modulePath)) {
			gameApplication_ = nullptr;
			gameScenesRegistered_ = false;
			return;
		}

		gameApplication_ = gameModule_.Create();
		gameScenesRegistered_ = false;
	}

	void RefreshPlaceToolPanel() {
		if(!engineUi_ || !gameApplication_) {
			return;
		}
		if(auto* panel = engineUi_->GetPlaceToolPanel()) {
			panel->RefreshPlaceItems();
		}
	}

	void RegisterGameScenesIfReady() {
		if(!sceneManager_ || !gameApplication_ || gameScenesRegistered_) {
			return;
		}

		// Project Browser 経由でプロジェクトを後から開いた場合は、FrameWork 側の RegisterScenes 呼び出しが既に終わっている。
		// そのため SceneManager が利用可能になった後でも、ゲーム DLL のシーンを明示的に登録する。
		Calyx::SceneRegistry registry(*sceneManager_);
		gameApplication_->RegisterScenes(registry);
		gameScenesRegistered_ = true;
	}

	void FinalizeGameApplication() {
		if(gameApplication_) {
			gameApplication_->OnFinalize();
			gameModule_.Destroy(gameApplication_);
			gameApplication_ = nullptr;
		}
		gameScenesRegistered_ = false;
	}

private:
	static constexpr CalyxEngine::SceneId kBlankSceneId = 0;
	static constexpr CalyxEngine::SceneId kDemoSceneId = 1;

	Calyx::ProjectInfo project_;
	CalyxEditor::ProjectBrowser projectBrowser_;
	CalyxEngine::SceneManager* sceneManager_ = nullptr;
	CalyxEngine::EngineUICore* engineUi_ = nullptr;
	GameModule gameModule_;
	Calyx::Application* gameApplication_ = nullptr;
	bool hasProject_ = false;
	bool gameScenesRegistered_ = false;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int) {
	CalyxEngine::RegisterGeneratedSceneObjects();
	CalyxEngine::RegisterGeneratedGameSceneObjects();

	EditorApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
