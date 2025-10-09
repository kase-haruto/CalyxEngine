#pragma once

////////////////////////////////////////////////////////////
//	include
////////////////////////////////////////////////////////////
#include <Engine/scene/Base/BaseScene.h>

/* objects */
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>

/* service */
#include <Game/UI/Controller/TitleMenuController.h>

/* c++ */
#include <array>
#include <memory>
class TitleScene final :
	public BaseScene {
  public:
	TitleScene();
	~TitleScene() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void CleanUp() override;
	void LoadAssets() override;
	void Draw([[maybe_unused]] ID3D12GraphicsCommandList* cmdList, class PipelineService* psoService, RenderTargetType type) override;

  private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject> modelField_ = nullptr;

	std::unique_ptr<TitleMenuController> menu_ = nullptr;

	/* func ======================================================*/
	std::function<void()> transitionForGameScene_;
	std::function<void()> endGameReqest_;
};
