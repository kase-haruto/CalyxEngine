#include <CalyxEngine/CalyxEngine.h>

#include <Engine/Scene/Test/TestScene.h>
#include <Game/Scene/Utility/SceneTypeUtil.h>

class GameApplication : public Calyx::Application {
public:
	void RegisterScenes(Calyx::SceneRegistry& registry) override {
		registry.AddScene<TestScene>(GameSceneUtil::ToSceneId(SceneType::TEST));
		registry.SetStartupScene(GameSceneUtil::ToSceneId(SceneType::TEST));
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
