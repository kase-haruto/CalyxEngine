#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */

// engine
#include <Engine/Objects/Event/BaseEventObject.h>

class EnemySpawner;

/**
 *	イベントを介して敵を発生させるためのイベント
 */
class EnemySpawnEvent
	: public BaseEventObject {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	EnemySpawnEvent();
	~EnemySpawnEvent() override;

	// 初期化
	void Initialize() override;
	void AlwaysUpdate(float dt) override;

	// 発火時処理
	void OnCollisionStay(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	//設定の保存ロード

	// debug ui
	void DerivativeGui() override;
	void ConfigGUi() override;

	// クラス名取得
	std::string_view GetTypeName() const override{return "EnemySpawnerEvent";};
private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	std::vector<EnemySpawner*> spawners_;
	float deltaTime_;
};
