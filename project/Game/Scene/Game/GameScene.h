#pragma once

////////////////////////////////////////////////////////////
//  include
////////////////////////////////////////////////////////////
/* engine */
#include <Engine/Graphics/Camera/Action/CameraTurnAroundAction.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Renderer/Sprite/Sprite.h>
#include <Engine/scene/Base/BaseScene.h>

/* game */
#include <Game/3d/GameCamera/RailCamera.h>
#include <Game/3dObject/Actor/Boss/Boss.h>
#include <Game/3dObject/Actor/Boss/Service/RailProgressBossSpawnService.h>
#include <Game/3dObject/Actor/Bullet/Container/EnemyBulletContainer.h>
#include <Game/3dObject/Actor/Enemy/BindingService/EnemyRuntimeBindingService.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Game/3dObject/Actor/Player/Player.h>
#include <Game/Runtime/Engagement/EnemyEngagementService.h>

/* c++ */
#include "Result/ResultOverlay.h"

#include <memory>

// 配線サービス
class EnemyRuntimeBindingService;
class ScoreService;
class NumbersSprite;
class CameraTurnAroundAction;

/*-----------------------------------------------------------------------------------------
 * GameScene
 * - ゲームの本編シーンクラス
 * - プレイヤー、敵、ボス、UI、カメラの管理と更新・描画を担当
 *---------------------------------------------------------------------------------------*/
class GameScene final
	: public BaseScene {
public:
	/**
	 * \brief コンストラクタ
	 */
	GameScene();
	/**
	 * \brief デストラクタ
	 */
	~GameScene() override;

	/**
	 * \brief 初期化
	 */
	void Initialize() override;
	/**
	 * \brief 更新処理
	 * \param dt デルタタイム
	 */
	void Update(float dt) override;
	/**
	 * \brief 描画処理
	 * \param cmdList コマンドリスト
	 * \param pipelineService パイプラインサービス
	 * \param renderTarget レンダリングターゲット
	 */
	void Draw(ID3D12GraphicsCommandList* cmdList, class PipelineService* pipelineService,  IRenderTarget* renderTarget ) override;
	/**
	 * \brief 終了処理
	 */
	void CleanUp() override;
	/**
	 * \brief アセット読み込み
	 */
	void LoadAssets() override;

	/**
	 * \brief 合計スコアを取得
	 * \return 合計スコア
	 */
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
	std::unique_ptr<ResultTransitionPayload> BuildResultPayload() const;
	/**
	 * \brief リザルトフェーズへ移行
	 */
	void EnterResultPhase();
	/**
	 * \brief スコア更新
	 */
	void ScoreUpdate() const;

private:
	/* objects ======================================================*/
	std::shared_ptr<BaseGameObject> modelField_;
	std::weak_ptr<Camera3d>			wMainCamera_;
	std::weak_ptr<Player>			wPlayer_;
	std::weak_ptr<Boss>				wBoss_;

	std::unique_ptr<EnemyBulletContainer> enemyBulletContainer_ = nullptr; //< 敵の弾コンテナ

	/* UIs ==========================================================*/
	std::unique_ptr<Sprite> shootUI_;
	std::unique_ptr<Sprite> aimUI_;
	std::unique_ptr<Sprite> avoidanceUI_;

	/* result services =============================================*/
	std::unique_ptr<ResultOverlay> resultOverlay_ = nullptr;		//< リザルトオーバーレイ
	std::unique_ptr<ResultTransitionPayload> resultPayload_;		//< リザルトペイロード
	
	/* runtime services =============================================*/
	int16_t totalScore_;

	std::unique_ptr<EnemyRuntimeBindingService>	  enemyBinding_;	//< 配線
	std::unique_ptr<EnemyEngagementService>		  enemyEngagement_; //<
	std::unique_ptr<RailProgressBossSpawnService> occurrenceBoss_;	//< カメラの進み具合でボスを発生
	std::shared_ptr<EnemyCollection>			  enemyCollection_;
	std::unique_ptr<ScoreService>				  score_;
	std::unique_ptr<NumbersSprite>				  numbersSprite_;

	// 更新関数ポインタ
	using UpdateFunc	   = void (GameScene::*)(float);
	UpdateFunc updateFunc_ = nullptr;
};