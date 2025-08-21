#include "EnemySpawner.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include <Game/Installer/Enemy/EnemyInstaller.h>
#include <Game/3dObject/Actor/Enemy/Directory/IEnemyDirectory.h>

EnemySpawner::EnemySpawner(const std::string& name) : SceneObject() {
	SetName(name, ObjectType::GameObject);
}

void EnemySpawner::Update(float dt) {
	//// 自己回転
	//Vector3 rot = rotationDir_ * rotationSpeed_ * dt;
	//worldTransform_.eulerRotation += rot;
	//worldTransform_.rotationSource = RotationSource::Euler;

	// スポーンタイマー
	spawnTimer_ += dt;
	if (spawnTimer_ >= spawnInterval_) {
		Spawn();
		spawnTimer_ = 0.0f;
	}

	// 死体回収（Scene からも除去）
	GarbageCollectDead();
}

void EnemySpawner::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();
	PrimitiveDrawer::GetInstance()->DrawSphere(worldTransform_.GetWorldPosition());
}

void EnemySpawner::ApplyConfig() {
	auto cfg = config_.GetConfig();
	if (!cfg.name.empty()) {
		SetName(cfg.name, ObjectType::GameObject);
	}
	worldTransform_.ApplyConfig(cfg.transform);
	rotationSpeed_ = cfg.rotationSpeed;
	rotationDir_ = cfg.rotationDir;
	spawnInterval_ = cfg.spawnInterval;
	spawnAreaMin_ = cfg.spawnAreaMin;
	spawnAreaMax_ = cfg.spawnAreaMax;
	maxSpawnCount_ = cfg.maxSpawnCount;
}

void EnemySpawner::ExtractConfig() {
	auto cfg = config_.GetConfig();
	cfg.name = GetName();
	cfg.rotationSpeed = rotationSpeed_;
	cfg.rotationDir = rotationDir_;
	cfg.spawnInterval = spawnInterval_;
	cfg.spawnAreaMin = spawnAreaMin_;
	cfg.spawnAreaMax = spawnAreaMax_;
	cfg.maxSpawnCount = maxSpawnCount_;
	cfg.transform = worldTransform_.ExtractConfig();
}

void EnemySpawner::ShowGui() {
	// Transform情報
	worldTransform_.ShowImGui();

	ImGui::DragFloat("Rotation Speed", &rotationSpeed_, 0.1f);
	ImGui::DragFloat3("Rotation Dir", &rotationDir_.x, 0.1f);
	ImGui::InputFloat("Spawn Interval", &spawnInterval_);
	ImGui::DragInt("Max Spawn Count", reinterpret_cast<int*>(&maxSpawnCount_), 1, 1, 100);
	ImGui::DragFloat3("Spawn Area Min", &spawnAreaMin_.x, 0.1f);
	ImGui::DragFloat3("Spawn Area Max", &spawnAreaMax_.x, 0.1f);

#ifdef _DEBUG
	ImGui::SeparatorText("Spawner Config");
	config_.ShowGui();
#endif
}

void EnemySpawner::SetPlayerTransform(WorldTransform* playerTransform) {
	playerTransform_ = playerTransform;
}

void EnemySpawner::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void EnemySpawner::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<EnemySpawner*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

void EnemySpawner::Spawn() {
	// 現在の生存数を数える（死体は別途回収）
	size_t aliveCount = 0;
	for (auto& e : spawnedEnemies_) {
		if (e && e->GetIsAlive()) ++aliveCount;
	}
	if (aliveCount >= maxSpawnCount_) return;

	// 新規スポーン
	EnemyInstaller installer;
	auto enemy = installer.InstallEnemy();
	if (!enemy) return;

	enemy->Initialize();
	enemy->SetPlayerTransform(playerTransform_); // null でも可（後で再配線され得る）

	// 位置と親子付け
	Vector3 localOffset = Random::GenerateVector3(spawnAreaMin_, spawnAreaMax_);
	enemy->SetPosition(localOffset);
	enemy->SetParent(&worldTransform_);

	// 自前リストに登録
	spawnedEnemies_.push_back(enemy);

	// ディレクトリにも登録（あれば）
	if (directory_) { directory_->Register(enemy); }
}

void EnemySpawner::GarbageCollectDead() {
	auto* lib = SceneContext::Current()->GetObjectLibrary();
	for (auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end(); ) {
		auto& e = *it;
		if (!e || !e->GetIsAlive()) {
			if (e) {
				// ディレクトリからも除去（任意：SnapshotAlive 側で自然回収でも可）
				if (directory_) directory_->Unregister(e.get());
				// シーンから除去
				if (lib) lib->RemoveObject(e);
			}
			it = spawnedEnemies_.erase(it);
		} else {
			++it;
		}
	}
}

REGISTER_SCENE_OBJECT(EnemySpawner)
