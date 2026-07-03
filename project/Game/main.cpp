#include <CalyxEngine/CalyxEngine.h>
#include <CalyxEngine/Project.h>
#include <Engine/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include <Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h>

#include "GameApplication.h"

#include <filesystem>
#include <memory>
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
		const std::string config = project.launchConfiguration.empty() ? "Develop" : project.launchConfiguration;
		if(config == "Debug" && !project.gameModuleDebug.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleDebug);
		if(config == "Develop" && !project.gameModuleDevelop.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleDevelop);
		if(config == "Release" && !project.gameModuleRelease.empty()) return Calyx::ResolveProjectPath(project, project.gameModuleRelease);
		return Calyx::ResolveProjectPath(project, project.gameModule);
	}

	class GameHostApplication : public Calyx::Application {
	public:
		~GameHostApplication() override {
			UnloadGameApplication();
		}

		void OnProjectLoaded(const Calyx::ProjectInfo& project) override {
			project_ = project;
			LoadGameApplication();
			ActiveApplication().OnProjectLoaded(project);
		}

		void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) override {
			ActiveApplication().OnSceneManagerReady(sceneManager);
		}

		void OnEngineUiReady(CalyxEngine::EngineUICore& engineUi) override {
			ActiveApplication().OnEngineUiReady(engineUi);
		}

		void OnInitialize() override {
			ActiveApplication().OnInitialize();
		}

		void OnUpdate() override {
			ActiveApplication().OnUpdate();
		}

		void OnRender() override {
			ActiveApplication().OnRender();
		}

		void OnFinalize() override {
			ActiveApplication().OnFinalize();
			UnloadGameApplication();
		}

		bool ShouldRenderEngineUi() const override {
			return ActiveApplication().ShouldRenderEngineUi();
		}

	private:
		Calyx::Application& ActiveApplication() const {
			return gameApplication_ ? *gameApplication_ : fallbackApplication_;
		}

		void LoadGameApplication() {
			UnloadGameApplication();
			if(!gameModule_.Load(SelectGameModulePath(project_))) {
				return;
			}
			gameApplication_ = gameModule_.Create();
		}

		void UnloadGameApplication() {
			if(gameApplication_) {
				gameModule_.Destroy(gameApplication_);
				gameApplication_ = nullptr;
			}
		}

		Calyx::ProjectInfo project_;
		mutable GameApplication fallbackApplication_;
		GameModule gameModule_;
		Calyx::Application* gameApplication_ = nullptr;
	};

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int){
	CalyxEngine::RegisterGeneratedSceneObjects();
	CalyxEngine::RegisterGeneratedGameSceneObjects();

	GameHostApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
