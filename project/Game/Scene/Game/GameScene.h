#pragma once

////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////
/* engine */
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>
#include <Engine/Graphics/Camera/Action/CameraTurnAroundAction.h>

/* game */
#include <Game/3d/GameCamera/RailCamera.h>
#include <Game/3dObject/Actor/Boss/Boss.h>
#include <Game/3dObject/Actor/Boss/Service/RailProgressBossSpawnService.h>
#include <Game/3dObject/Actor/Enemy/BindingService/EnemyRuntimeBindingService.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/Runtime/Engagement/EnemyEngagementService.h>
#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>

/* c++ */
#include <memory>

// 配線サービス
class EnemyRuntimeBindingService;
class ScoreService;
class NumbersSprite;
class CameraTurnAroundAction;

class GameScene final
	: public BaseScene {
public:
	GameScene();
	~GameScene() override;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(ID3D12GraphicsCommandList*,class PipelineService*,RenderTargetType) override;
	void CleanUp() override;
	void LoadAssets() override;


	int16_t GetTotalScore() const { return totalScore_; }

private:
	//===============================================================*/
	//                    private methods
	//===============================================================*/
	/**
	 * \brief プレイ中更新
	 */
	void PlayingUpdate(float dt);
	/**
	 * \brief 結果画面更新
	 */
	void ResultUpdate(float dt);
	/**
	 * \brief 結果ペイロードの構築
	 */
	void BuildResultPayload()const;
	/**
	 * \brief スコア更新
	 */
	void ScoreUpdate()const;

private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::weak_ptr<Camera3d>         wMainCamera_;
	std::weak_ptr<Player>           wPlayer_;
	std::weak_ptr<Boss>             wBoss_;

	std::unique_ptr<EnemyBulletContainer> enemyBulletContainer_ = nullptr; //< 敵の弾コンテナ

	/* UIs ==========================================================*/
	std::unique_ptr<Sprite> shootUI_;
	std::unique_ptr<Sprite> aimUI_;
	std::unique_ptr<Sprite> avoidanceUI_;

	/* runtime services =============================================*/
	int16_t totalScore_;

	std::unique_ptr<EnemyRuntimeBindingService>   enemyBinding_;    //< 配線
	std::unique_ptr<EnemyEngagementService>       enemyEngagement_; //<
	std::unique_ptr<RailProgressBossSpawnService> occurrenceBoss_;  //< カメラの進み具合でボスを発生
	std::shared_ptr<EnemyCollection>              enemyCollection_;
	std::unique_ptr<ScoreService>                 score_;
	std::unique_ptr<NumbersSprite>                numbersSprite_;

	// 更新関数ポインタ
	using UpdateFunc = void (GameScene::*)(float);
	UpdateFunc updateFunc_ = nullptr;
};