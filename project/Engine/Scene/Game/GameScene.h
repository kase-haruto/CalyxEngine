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
#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/Battle/Shooting/ShootingController/PlayerShootingController.h>

/* c++ */
#include <vector>
#include <memory>
#include <array>

//デバッグ関連//
#ifdef _DEBUG
#include<Externals/imgui/imgui.h>
#endif // _DEBUG

class GameScene final :
	public BaseScene{
public:
	GameScene();
	~GameScene() override = default;

	void Initialize()override;
	void Update()override;
	void Draw(ID3D12GraphicsCommandList*, class PipelineService*, RenderTargetType)override;
	void CleanUp()override;
	void LoadAssets()override;

private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject>   modelField_;
	std::shared_ptr<Player> player_;
	std::shared_ptr<EnemyCollection>  enemyCollection_;


	std::unique_ptr<RailCamera> railCamera_ = nullptr;
};

