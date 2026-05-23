#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Extensions/Fog/FogEffect.h>
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Objects/2D/Object2d/SpriteObject2d.h>
#include <Engine/Objects/2D/Animation/SpriteAnimator2d.h>
#include <Engine/Objects/3D/Actor/TestObject/CalyxHuman.h>
/* c++ */
#include <memory>
#include <vector>

#include <Game\Stage\Stage.h>
#include <Game\DemoPlayer\DemoPlayer.h>
#include <Game\StageGimmick\Manager\StageGimmickManager.h>
#include <Game/3d/Camera/RailCamera.h>
#include <Game\Scene\Transition\TransitionPayload.h>
#include <Game\Scene\Game\GameTransitionPayload.h>
#include <Game/Scene/Transition/TransitionControl.h>

///デバッグ関連///
#ifdef _DEBUG

#include <externals/imgui/imgui.h>
#endif // _DEBUG

/* ========================================================================
/* testScene
/* ===================================================================== */
class TestScene final :
	public BaseScene{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	TestScene();
	~TestScene() override = default;

	void Initialize()override;
	void Update(float dt)override;
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, IRenderTarget* )override;
	void CleanUp()override;
	void LoadAssets()override;

	void OnPayload(std::unique_ptr<CalyxEngine::IScenePayload> payload) override;

private:

	void InitPauseResource();
	void CheckStageState(float dt);
	void PauseUpdate(float dt);
	void PauseUIUpdate(float dt);
	void PauseOpen();
	void PauseClose();
	void ButtonFadeIn(float dt);
	void ButtonFadeOut(float dt);
	std::unique_ptr<TransitionPayload> BuildNowTypePayload(SceneType Type);
	std::unique_ptr<GameTransitionPayload> BuildGamePayload(int num);

private:
	/* graphics =====================================================*/
	std::unique_ptr<FogEffect>fog_ = nullptr;


	/* objects ====================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::unique_ptr<Sprite> testSprite_;
	std::shared_ptr<CalyxHuman> animationHuman_;
	std::shared_ptr<DemoPlayer>		player_;

	std::unique_ptr<CalyxEngine::SpriteObject2d> fanBg_		  = nullptr;
	std::unique_ptr<CalyxEngine::SpriteAnimator2d> fanAnim_		  = nullptr;
	std::unique_ptr<CalyxEngine::SpriteObject2d>   resumeBtn_	= nullptr;
	std::unique_ptr<CalyxEngine::SpriteObject2d>   toSelectBtn_	  = nullptr;
	std::unique_ptr<CalyxEngine::SpriteObject2d>   toTitleBtn_	  = nullptr;
	
	int									 stageNum_			  = 0;
	std::unique_ptr<Stage>				 stage_				  = nullptr;
	std::unique_ptr<StageGimmickManager> stageGimmickManager_ = nullptr;

	std::unique_ptr<TransitionControl> transitionControl_ = nullptr;

	std::unique_ptr<CalyxEngine::IScenePayload> payload_;

	SceneType preType_	 = SceneType::SELECT;

	float lastDt_			  = 0.0f;
	int	  selectedIndex_	  = 0;

	// ポーズ用（ボタン）
	float fadeTime_		   = 0.0f;
	float currentFadeTime_ = 0.0f;
	bool  isFadeFinish_	   = false;
	
	// ポーズ用（扇子）
	float openingTime_		  = 0.0f;
	float currentOpeningTime_ = 0.0f;
	bool  isFanOpen_		  = false;
	bool  isOncePlay_		  = false;
	bool  isPaused_			  = false;
	// 遷移用
	bool IsPhase_	= false;
	bool IsOpening_ = false;
};