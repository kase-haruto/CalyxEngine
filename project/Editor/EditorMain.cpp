#include "ProjectBrowser.h"

#include <CalyxEngine/CalyxEngine.h>

#include <Engine/Scene/Test/TestScene.h>

#include <vector>

class EditorApplication : public Calyx::Application {
public:
	void OnProjectLoaded(const Calyx::ProjectInfo& project) override {
		project_ = project;
		hasProject_ = true;

		std::vector<Calyx::RecentProjectEntry> recentProjects;
		const auto							  registryPath = Calyx::DefaultProjectRegistryPath();
		Calyx::LoadRecentProjects(registryPath, recentProjects);
		Calyx::AddRecentProject(recentProjects, project_);
		Calyx::SaveRecentProjects(registryPath, recentProjects);
	}

	void RegisterScenes(Calyx::SceneRegistry& registry) override {
		constexpr CalyxEngine::SceneId startupScene = 0;
		registry.AddScene<TestScene>(startupScene);
		registry.SetStartupScene(startupScene);
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
	Calyx::ProjectInfo project_;
	CalyxEditor::ProjectBrowser projectBrowser_;
	bool hasProject_ = false;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int) {
	EditorApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
