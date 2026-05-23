#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Extensions/Fog/FogEffect.h>
#include <Engine/Objects/3D/Actor/TestObject/CalyxHuman.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Scene/Transitioner/IScenePayload.h>
/* c++ */
#include <memory>
#include <vector>


#include <Game\Scene\Game\GameTransitionPayload.h>
#include <Game/Scene/Transition/TransitionControl.h>

/// デバッグ関連///
#ifdef _DEBUG

#include <externals/imgui/imgui.h>
#endif // _DEBUG

/* ========================================================================
/* SelectScene
/* ===================================================================== */
class SelectScene final : public BaseScene {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	SelectScene();
	~SelectScene() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, IRenderTarget*) override;
	void CleanUp() override;
	void LoadAssets() override;

	void OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) override;

private:

	void SelectUpdate(float dt);
	void PhaseUpdate(float dt);
	std::unique_ptr<GameTransitionPayload> BuildGamePayload(int num);

private:
	/* graphics =====================================================*/
	std::unique_ptr<FogEffect> fog_ = nullptr;

	/* objects ====================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::unique_ptr<Sprite>			testSprite_;
	std::shared_ptr<CalyxHuman>		animationHuman_;

	std::unique_ptr<Sprite> pauseBg_ = nullptr;
	std::unique_ptr<TransitionControl> transitionControl_ = nullptr;

	std::unique_ptr<CalyxEngine::IScenePayload> gamePayload_;
	int selectedIndex_ = 0;

	SceneType preType_	 = SceneType::TITLE;
	bool	  IsPhase_	 = false;
	bool	  IsOpening_ = false;
};
