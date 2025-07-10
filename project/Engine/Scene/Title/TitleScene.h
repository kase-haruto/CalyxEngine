#pragma once

////////////////////////////////////////////////////////////
//	include
////////////////////////////////////////////////////////////
#include <Engine/scene/Base/BaseScene.h>

/* objects */
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>

/* graphics */
#include <Engine/Objects/LightObject/DirectionalLight.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Game/3d/GameCamera/RailCamera.h>
#include <Engine/Extensions/SkyBox/SkyBox.h>

/* object */
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>

/* c++ */
#include <vector>
#include <memory>
#include <array>

//デバッグ関連//
#ifdef _DEBUG
#include<Externals/imgui/imgui.h>
#endif // _DEBUG

class TitleScene final :
	public BaseScene {
public:
	TitleScene();
	~TitleScene() override = default;

	void Initialize()override;
	void Update()override;
	void CleanUp()override;
	void LoadAssets()override;
	void Draw([[maybe_unused]] ID3D12GraphicsCommandList* cmdList, class PipelineService* psoService,RenderTargetType type)override;
private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject> modelField_ = nullptr;

	std::unique_ptr<Sprite> title_;
};

