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
#include <Game/3d/GameCamera/RailCamera.h>

/* c++ */
#include <memory>

class GameScene final :
	public BaseScene{
public:
	GameScene();
	~GameScene() override = default;

	void Initialize()override;
	void Update(float dt)override;
	void Draw(ID3D12GraphicsCommandList*, class PipelineService*, RenderTargetType)override;
	void CleanUp()override;
	void LoadAssets()override;

private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject>   modelField_;
	std::shared_ptr<Player> player_;
	std::shared_ptr<EnemyCollection>  enemyCollection_;

	std::shared_ptr<RailCamera> railCamera_ = nullptr;

	std::weak_ptr<Player> wPlayer_;
	std::weak_ptr<EnemyCollection> wEnemyCol_;

	std::unique_ptr<Sprite> attackSprite_;					//< 攻撃状態スプライト

};

