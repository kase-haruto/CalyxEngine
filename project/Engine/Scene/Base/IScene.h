#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Graphics/RenderTarget/Detail/RenderTargetDetail.h>

// forward declaration
namespace CalyxGraphics {
	class DxCore;

	
}
class BaseCamera;
class SceneContext;

namespace CalyxScene {
	class ISceneTransitionRequestor;
}

/*-----------------------------------------------------------------------------------------
 * IScene
 * - シーンインターフェース
 * - シーンの初期化・更新・描画・遷移処理の共通インターフェースを定義
 *---------------------------------------------------------------------------------------*/
class IScene{
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	IScene();
	IScene(CalyxGraphics::DxCore* dxCore);
	virtual ~IScene() = default;

	virtual void Initialize() = 0;
	virtual void Update(float dt) = 0;
	virtual void PostUpdate(ID3D12GraphicsCommandList* cmdList,
							class PipelineService* psoService) = 0;
	virtual void Draw([[maybe_unused]]ID3D12GraphicsCommandList* cmdList,
					  [[maybe_unused]] class PipelineService*,
					  [[maybe_unused]] class IRenderTarget* rt){}
	virtual void DrawSpritesOnly([[maybe_unused]] ID3D12GraphicsCommandList* cmdList,
								 [[maybe_unused]] class PipelineService* psoService) {}
	virtual void CleanUp() = 0;
	virtual void LoadAssets() = 0;
	virtual void SetTransitionRequestor(CalyxScene::ISceneTransitionRequestor* requestor) = 0;
	//--------- accessor -----------------------------------------------------
	virtual SceneContext* GetSceneContext() const = 0;
	virtual void InjectContext([[maybe_unused]]SceneContext* ctx) {};

	virtual void OnExit(){}
	virtual void OnEnter(){}

	bool GetIsEndGame()const { return isEndGame_; };
	void GameEndReqest() { isEndGame_ = true; }
protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	CalyxGraphics::DxCore* pDxCore_ = nullptr;

	bool isEndGame_ = false;
};
