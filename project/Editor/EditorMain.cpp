#include "ProjectBrowser.h"

#include <CalyxEngine/CalyxEngine.h>

#include <Demo/Scene/DemoScene/DemoScene.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include <Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h>
#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/System/SceneManager.h>

#include <Windows.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

	class GameModule {
	public:
		using CreateApplicationFn = Calyx::Application* (*)();
		using DestroyApplicationFn = void (*)(Calyx::Application*);

		~GameModule() {
			Unload();
		}

		bool Load(const Calyx::ProjectInfo& project) {
			Unload();

			const auto selectedModule = SelectGameModule(project);
			if(selectedModule.empty()) {
				return false;
			}

			// ゲーム DLL を Editor プロセス内にロードする。
			// これにより Visual Studio は Editor 実行中でもゲーム側 PDB を読み込み、
			// ゲーム C++ コード上のブレークポイントを解決できる。
			const auto modulePath = Calyx::ResolveProjectPath(project, selectedModule);
			if(!std::filesystem::exists(modulePath)) {
				return false;
			}

			module_ = LoadLibraryW(modulePath.wstring().c_str());
			if(!module_) {
				return false;
			}

			create_ = reinterpret_cast<CreateApplicationFn>(GetProcAddress(module_, "CreateCalyxApplication"));
			destroy_ = reinterpret_cast<DestroyApplicationFn>(GetProcAddress(module_, "DestroyCalyxApplication"));
			if(!create_ || !destroy_) {
				Unload();
				return false;
			}

			application_ = create_();
			if(!application_) {
				Unload();
				return false;
			}

			return true;
		}

		void Unload() {
			if(application_ && destroy_) {
				destroy_(application_);
			}
			application_ = nullptr;
			create_ = nullptr;
			destroy_ = nullptr;

			if(module_) {
				FreeLibrary(module_);
				module_ = nullptr;
			}
		}

		Calyx::Application* GetApplication() const {
			return application_;
		}

	private:
		std::filesystem::path SelectGameModule(const Calyx::ProjectInfo& project) const {
			// Visual Studio から起動された場合は --config で構成名が渡される。
			// Project Browser から直接開いた場合は構成名が無いため、デバッグしやすい Debug を既定にする。
			const std::string config = project.launchConfiguration.empty() ? "Debug" : project.launchConfiguration;

			// 構成別パスが設定されていればそれを優先する。
			// 未設定の古い .calyxproj では最後に gameModule へフォールバックする。
			if(config == "Debug" && !project.gameModuleDebug.empty()) {
				return project.gameModuleDebug;
			}
			if(config == "Develop" && !project.gameModuleDevelop.empty()) {
				return project.gameModuleDevelop;
			}
			if(config == "Release" && !project.gameModuleRelease.empty()) {
				return project.gameModuleRelease;
			}
			return project.gameModule;
		}

		HMODULE module_ = nullptr;
		CreateApplicationFn create_ = nullptr;
		DestroyApplicationFn destroy_ = nullptr;
		Calyx::Application* application_ = nullptr;
	};

} // namespace

class EditorApplication : public Calyx::Application {
public:
	void OnProjectLoaded(const Calyx::ProjectInfo& project) override {
		project_ = project;
		hasProject_ = true;
		gameScenesRegistered_ = false;
		Calyx::SetCurrentProject(project_);
		AssetDatabase::GetInstance()->Initialize(Calyx::GetAssetRoot());
		gameModule_.Load(project_);
		if(auto* gameApplication = gameModule_.GetApplication()) {
			gameApplication->OnProjectLoaded(project_);
		}

		std::vector<Calyx::RecentProjectEntry> recentProjects;
		const auto							  registryPath = Calyx::DefaultProjectRegistryPath();
		Calyx::LoadRecentProjects(registryPath, recentProjects);
		Calyx::AddRecentProject(recentProjects, project_);
		Calyx::SaveRecentProjects(registryPath, recentProjects);

		RegisterGameScenesIfReady();
	}

	void RegisterScenes(Calyx::SceneRegistry& registry) override {
		registry.AddScene<BaseScene>(kBlankSceneId);
		registry.AddScene<DemoScene>(kDemoSceneId);
		registry.SetStartupScene(kBlankSceneId);
		if(auto* gameApplication = gameModule_.GetApplication()) {
			gameApplication->RegisterScenes(registry);
			gameScenesRegistered_ = true;
		}
	}

	void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) override {
		sceneManager_ = &sceneManager;
		RegisterGameScenesIfReady();
	}

	void OnUpdate() override {
		if(hasProject_) {
			if(auto* gameApplication = gameModule_.GetApplication()) {
				gameApplication->OnUpdate();
			}
			return;
		}

		Calyx::ProjectInfo selectedProject;
		if(projectBrowser_.Draw(selectedProject)) {
			OnProjectLoaded(selectedProject);
		}
	}

	void OnRender() override {
		if(auto* gameApplication = gameModule_.GetApplication()) {
			gameApplication->OnRender();
		}
	}

	void OnFinalize() override {
		if(auto* gameApplication = gameModule_.GetApplication()) {
			gameApplication->OnFinalize();
		}
	}

	bool ShouldRenderEngineUi() const override {
		return hasProject_;
	}

private:
	void RegisterGameScenesIfReady() {
		if(!sceneManager_ || !hasProject_) {
			return;
		}

		if(auto* gameApplication = gameModule_.GetApplication()) {
			if(gameScenesRegistered_) {
				gameApplication->OnSceneManagerReady(*sceneManager_);
				return;
			}
			Calyx::SceneRegistry registry(*sceneManager_);
			gameApplication->RegisterScenes(registry);
			gameScenesRegistered_ = true;
			gameApplication->OnSceneManagerReady(*sceneManager_);
			return;
		}

		if(project_.templateName == "Demo") {
			sceneManager_->SetCurrent(kDemoSceneId);
			return;
		}

		sceneManager_->SetCurrent(kBlankSceneId);
	}

private:
	static constexpr CalyxEngine::SceneId kBlankSceneId = 0;
	static constexpr CalyxEngine::SceneId kDemoSceneId = 1;

	Calyx::ProjectInfo project_;
	CalyxEditor::ProjectBrowser projectBrowser_;
	GameModule gameModule_;
	CalyxEngine::SceneManager* sceneManager_ = nullptr;
	bool hasProject_ = false;
	bool gameScenesRegistered_ = false;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int) {
	CalyxEngine::RegisterGeneratedSceneObjects();
	CalyxEngine::RegisterGeneratedGameSceneObjects();

	EditorApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
