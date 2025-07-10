#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
/* engine */
#include <Engine/Extensions/Fog/FogEffect.h>
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
/* c++ */
#include <memory>
#include <vector>

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
	void Update()override;
	void Draw(ID3D12GraphicsCommandList* cmdLst, class PipelineService* psoService, RenderTargetType type)override;
	void CleanUp()override;
	void LoadAssets()override;
private:
	/* graphics =====================================================*/
	std::unique_ptr<FogEffect>fog_ = nullptr;

	/* objects ====================================================*/
	std::unique_ptr<BaseGameObject> testObject;
	std::unique_ptr<Sprite> testSprite_;
	std::shared_ptr<BaseGameObject> animationHuman_;
};

