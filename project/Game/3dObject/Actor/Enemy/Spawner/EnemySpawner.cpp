#include "EnemySpawner.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

#include <externals/imgui/ImGuiFileDialog.h>

EnemySpawner::EnemySpawner(const std::string& name) {
	SetName(name, ObjectType::GameObject);
	worldTransform_.Initialize();
	worldTransform_.translation = { 0.0f, 0.0f, 5.0f };

}

void EnemySpawner::Update(float dt) {
	Vector3 rot = rotationDir_ * rotationSpeed_ * dt;
	worldTransform_.eulerRotation += rot;

	worldTransform_.rotationSource = RotationSource::Euler;

	spawnTimer_ += dt;
	if (spawnTimer_ >= spawnInterval_) {
		Spawn();
		spawnTimer_ = 0.0f;
	}
}

void EnemySpawner::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();

	PrimitiveDrawer::GetInstance()->DrawSphere(worldTransform_.GetWorldPosition());
}

void EnemySpawner::ApplyConfig() {
	auto cfg = config_.GetConfig();

	SetName(cfg.name, ObjectType::GameObject);
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
}

void EnemySpawner::ShowGui() {
	if (ImGui::TreeNode(GetName().c_str())) {
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

		ImGui::TreePop();
	}
}


void EnemySpawner::SetPlayerTransform(WorldTransform* playerTransform) {
	playerTransform_ = playerTransform;
	worldTransform_.parent = playerTransform->parent;
}

void EnemySpawner::Spawn() {
	if (!ownerCollection_) return;

	// 有効な敵だけ残す
	size_t aliveCount = 0;
	for (auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end();) {
		if (!(*it) || !(*it)->GetIsAlive()) {
			it = spawnedEnemies_.erase(it);
		} else {
			++it;
			++aliveCount;
		}
	}

	if (aliveCount >= maxSpawnCount_) return;

	// 新規スポーン
	auto enemy = SceneAPI::Instantiate<Enemy>("ghost.obj", "enemy");
	enemy->Initialize();
	if (!enemy) return;

	Vector3 localOffset = Random::GenerateVector3(spawnAreaMin_, spawnAreaMax_);
	enemy->SetPosition(localOffset);
	enemy->SetParent(&worldTransform_);

	ownerCollection_->AddEnemy(enemy);
	spawnedEnemies_.push_back(enemy);
}
