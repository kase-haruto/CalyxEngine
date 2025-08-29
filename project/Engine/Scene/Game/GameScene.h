#pragma once

////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////
/* engine */
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>

/* game */
#include <Game/3d/GameCamera/RailCamera.h>
#include <Game/3dObject/Actor/Boss/Service/RailProgressBossSpawnService.h>
#include <Game/3dObject/Actor/Enemy/BindingService/EnemyRuntimeBindingService.h>
#include <Game/Runtime/Engagement/EnemyEngagementService.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Game/3dObject/Actor/Player/Player.h>

/* c++ */
#include <memory>

// 前方宣言（配線サービス）
class EnemyRuntimeBindingService;

class GameScene final : public BaseScene {
public:
	GameScene();
	~GameScene() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList*, class PipelineService*, RenderTargetType) override;
	void CleanUp() override;
	void LoadAssets() override;

private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::shared_ptr<Player>         player_;
	std::shared_ptr<RailCamera>     railCamera_ = nullptr;
	std::weak_ptr<Player>           wPlayer_;
	std::unique_ptr<Sprite>         attackSprite_;

	/* runtime services =============================================*/
	std::unique_ptr<EnemyRuntimeBindingService> enemyBinding_;		//< 配線
	std::unique_ptr<EnemyEngagementService> enemyEngagement_;		//< 
	std::unique_ptr<RailProgressBossSpawnService> occurrenceBoss_;	//< カメラの進み具合でボスを発生

	std::shared_ptr<EnemyCollection>  enemyCollection_;
};
