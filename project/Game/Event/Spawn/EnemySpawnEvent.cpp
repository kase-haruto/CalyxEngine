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

EnemySpawnEvent::EnemySpawnEvent(const std::string& name) : BaseEventObject(name) {
	baseConfig_.SetOnApplied([this](const EventConfig&) {
		this->ApplyConfig();
	});

	baseConfig_.SetOnExtracted([this](const EventConfig&) {
		this->ExtractConfig();
	});
}

EnemySpawnEvent::~EnemySpawnEvent() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期か
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::Initialize() {
	const std::string configRoot = "Event/";
	baseConfig_.LoadConfig(configRoot + GetName());

	collider_->SetColor(Vector3(0, 1, 0));

}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::AlwaysUpdate(float dt) {
	BaseEventObject::AlwaysUpdate(dt);
	deltaTime_ = dt;

	if (spawners_.empty()) {
		for (auto& child : GetChildren()) {
			if (auto spawner = std::dynamic_pointer_cast<EnemySpawner>(child)) {
				spawners_.push_back(spawner.get());
			}
		}
	}
}
void EnemySpawnEvent::OnCollisionEnter(Collider*) {

	if (spawners_.empty()) {
		for (auto& child : GetChildren()) {
			if (auto spawner = std::dynamic_pointer_cast<EnemySpawner>(child)) {
				spawners_.push_back(spawner.get());
			}
		}
	}

	for (auto* spawner : spawners_) {
		if (spawner) {
			spawner->SpawnAllImmediate();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		衝突中
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::OnCollisionStay(Collider*) {
	// for(auto* spawner : spawners_) {
	// 	if(spawner) {
	// 		spawner->TickSpawnTimer(deltaTime_);
	// 	}
	// }
}

/////////////////////////////////////////////////////////////////////////////////////////
//		離れた時
/////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawnEvent::OnCollisionExit(Collider*) {

	if (spawners_.empty()) {
		for (auto& child : GetChildren()) {
			if (auto spawner = std::dynamic_pointer_cast<EnemySpawner>(child)) {
				spawners_.push_back(spawner.get());
			}
		}
	}

	auto copy = spawners_;

	for (auto* spawner : copy) {
		if (spawner) {
			spawner->DissolveFormation();
		}
	}

	spawners_.clear();
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