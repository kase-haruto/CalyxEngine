#include "EnemySpawnEvent.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>

REGISTER_SCENE_OBJECT(EnemySpawnEvent);

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor
/////////////////////////////////////////////////////////////////////////////////////////
EnemySpawnEvent::EnemySpawnEvent() {
	SceneObject::SetName("EnemySpawnEvent", ObjectType::Event);
	baseConfig_.SetOnApplied([this](const EventConfig&) {
		this->ApplyConfig();
	});

	baseConfig_.SetOnExtracted([this](const EventConfig&) {
		this->ExtractConfig();
	});
}

EnemySpawnEvent::EnemySpawnEvent(const std::string& name)
	: BaseEventObject(name) {
	baseConfig_.SetOnApplied([this](const EventConfig&) {
		this->ApplyConfig();
	});

	baseConfig_.SetOnExtracted([this](const EventConfig&) {
		this->ExtractConfig();
	});
}

EnemySpawnEvent::~EnemySpawnEvent() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::Initialize() {
	const std::string configRoot = "Event/";
	baseConfig_.LoadConfig(configRoot + GetName());

	collider_->SetColor(Vector3(0, 1, 0));

	// シーンロード直後に一度収集
	CollectSpawners();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt);
	deltaTime_ = dt;

	// 子構成が変わる可能性があるなら、空のときは取り直す
	if(spawners_.empty()) {
		CollectSpawners();
	}
}

void EnemySpawnEvent::OnCollisionEnter(Collider*) {
	// 毎回、最新の子 spawner を取り直す
	CollectSpawners();

	for(auto* spawner : spawners_) {
		if(spawner) {
			spawner->SpawnAllImmediate();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突中
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::OnCollisionStay(Collider*) {
	// 必要であれば個別スポーン制御を復活させる
	// CollectSpawners(*this);
	// for (auto* spawner : spawners_) {
	//     if (spawner) {
	//         spawner->TickSpawnTimer(deltaTime_);
	//     }
	// }
}

/////////////////////////////////////////////////////////////////////////////////////////
//		離れた時
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::OnCollisionExit(Collider*) {

	OutputDebugStringA("[EnemySpawnEvent] OnCollisionExit called\n");

	CollectSpawners();

	for(auto* spawner : spawners_) {
		if(spawner) {
			spawner->DissolveFormation();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		parameter調整
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::DerivativeGui() {
	BaseEventObject::DerivativeGui();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		保存・ロード
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::ConfigGUi() {
	BaseEventObject::ConfigGUi();
}

void EnemySpawnEvent::CollectSpawners() {
	spawners_.clear();

	for(auto& child : GetChildren()) {
		if(auto spawner = std::dynamic_pointer_cast<EnemySpawner>(child)) {
			spawners_.push_back(spawner.get());
		}
	}
}