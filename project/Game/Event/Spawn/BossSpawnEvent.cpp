#include "BossSpawnEvent.h"
/* ========================================================================
/*      include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Scene/Context/SceneContext.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>

REGISTER_SCENE_OBJECT(BossSpawnEvent)

/////////////////////////////////////////////////////////////////////////
//		コンストラクタ / デストラクタ
/////////////////////////////////////////////////////////////////////////
BossSpawnEvent::BossSpawnEvent() {
	SceneObject::SetName("BossSpawnEvent",ObjectType::Event);
}
BossSpawnEvent::BossSpawnEvent(const std::string& name)
: BaseEventObject(name) {
}
BossSpawnEvent::~BossSpawnEvent() =default;

/////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////
void BossSpawnEvent::Initialize() {
	BaseEventObject::Initialize();
	// 色を紫に
	if(collider_) collider_->SetColor(CalyxMath::Vector4(1, 0, 1));
}

/////////////////////////////////////////////////////////////////////////
//		常時更新
/////////////////////////////////////////////////////////////////////////
void BossSpawnEvent::OnCollisionEnter(Collider* other) {
	(void)other;
	// プレイヤーが入ったらスポーン
	auto ctx = SceneContext::Current();
	if(!ctx || !ctx->GetObjectLibrary()) {
		return;
	}

	auto spawners = ctx->GetObjectLibrary()->FindByType<BossSpawner>();
	if(spawners.empty()) {
		return;
	}

	const Actor* target = nullptr;
	if(auto player = ctx->FindFirst<Player>()) {
		target = player.get();
	}

	for(const auto& spawner : spawners) {
		if(!spawner || spawner->WasSpawned()) {
			continue;
		}
		spawner->SetPlayerTransform(target);
		spawner->Spawn();
	}
}
void BossSpawnEvent::DerivativeGui() {
	BaseEventObject::DerivativeGui();
}