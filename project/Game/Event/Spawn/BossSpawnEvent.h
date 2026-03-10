#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Game/3dObject/Actor/Boss/Spawner/BossSpawner.h>

/*-----------------------------------------------------------------------------------------
* BossSpawnEvent
* - Bossをスポーンさせるイベント
* - ボスをスポーンさせたときの演出を入れるためのイベント
*---------------------------------------------------------------------------------------*/
class BossSpawnEvent final
: public BaseEventObject {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BossSpawnEvent();
	BossSpawnEvent(const std::string& name);
	~BossSpawnEvent() override;

	// 初期化
	void Initialize() override;

	// 発火時処理
	void OnCollisionEnter(Collider* other) override;

	// debugUI
	void DerivativeGui() override;
	
	// クラス名取得
	std::string_view GetTypeName() const override{return "BossSpawnEvent";};


private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
};

