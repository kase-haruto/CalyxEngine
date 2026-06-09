#include "ProjectBrowser.h"

#include <CalyxEngine/CalyxEngine.h>

#include <Demo/Scene/DemoScene/DemoScene.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Foundation/Reflection/CalyxGameObjectRegistry.generated.h>
#include <Engine/Foundation/Reflection/CalyxObjectRegistry.generated.h>
#include <Engine/Scene/Base/BaseScene.h>
#include <Engine/Scene/System/SceneManager.h>

#include <vector>

class EditorApplication : public Calyx::Application {
public:
	void OnProjectLoaded(const Calyx::ProjectInfo& project) override {
		project_ = project;
		hasProject_ = true;
		Calyx::SetCurrentProject(project_);
		AssetDatabase::GetInstance()->Initialize(Calyx::GetAssetRoot());

		std::vector<Calyx::RecentProjectEntry> recentProjects;
		const auto							  registryPath = Calyx::DefaultProjectRegistryPath();
		Calyx::LoadRecentProjects(registryPath, recentProjects);
		Calyx::AddRecentProject(recentProjects, project_);
		Calyx::SaveRecentProjects(registryPath, recentProjects);

		ApplyProjectTemplateScene();
	}

	void RegisterScenes(Calyx::SceneRegistry& registry) override {
		registry.AddScene<BaseScene>(kBlankSceneId);
		registry.AddScene<DemoScene>(kDemoSceneId);
		registry.SetStartupScene(kBlankSceneId);
	}

	void OnSceneManagerReady(CalyxEngine::SceneManager& sceneManager) override {
		sceneManager_ = &sceneManager;
		ApplyProjectTemplateScene();
	}

	void OnUpdate() override {
		if(hasProject_) {
			return;
		}

		Calyx::ProjectInfo selectedProject;
		if(projectBrowser_.Draw(selectedProject)) {
			OnProjectLoaded(selectedProject);
		}
	}

	bool ShouldRenderEngineUi() const override {
		return hasProject_;
	}

private:
	void ApplyProjectTemplateScene() {
		if(!sceneManager_ || !hasProject_) {
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
	CalyxEngine::SceneManager* sceneManager_ = nullptr;
	bool hasProject_ = false;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int) {
	CalyxEngine::RegisterGeneratedSceneObjects();
	CalyxEngine::RegisterGeneratedGameSceneObjects();

	EditorApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
