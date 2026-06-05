#include <CalyxEngine/CalyxEngine.h>

#include <Demo/Scene/Utility/SceneTypeUtil.h>
#include <Demo/Scene/DemoScene/DemoScene.h>

class GameApplication : public Calyx::Application {
public:
	void RegisterScenes(Calyx::SceneRegistry& registry) override {
		registry.AddScene<DemoScene>(GameSceneUtil::ToSceneId(SceneType::DEMO));
		registry.SetStartupScene(GameSceneUtil::ToSceneId(SceneType::DEMO));
	}

	void OnInitialize() override {}
	void OnUpdate() override {}
	void OnRender() override {}
	void OnFinalize() override {}
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR commandLine, int){
	GameApplication application;
	return Calyx::Run(hInstance, application, commandLine);
}
