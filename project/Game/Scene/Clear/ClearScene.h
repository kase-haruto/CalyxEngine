#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Extensions/Fog/FogEffect.h>
#include <Engine/Objects/3D/Actor/TestObject/CalyxHuman.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>
/* c++ */
#include <memory>
#include <vector>

#include <Game\DemoPlayer\DemoPlayer.h>

/// デバッグ関連///
#ifdef _DEBUG

#include <externals/imgui/imgui.h>
#endif // _DEBUG

/* ========================================================================
/* ClearScene
/* ===================================================================== */
class ClearScene final : public BaseScene {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	ClearScene();
	~ClearScene() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, IRenderTarget*) override;
	void CleanUp() override;
	void LoadAssets() override;

private:
	/* graphics =====================================================*/
	std::unique_ptr<FogEffect> fog_ = nullptr;

	/* objects ====================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::unique_ptr<Sprite>			testSprite_;
	std::shared_ptr<CalyxHuman>		animationHuman_;

	std::unique_ptr<Sprite> pauseBg_  = nullptr;
};
