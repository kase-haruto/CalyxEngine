#include "EnemySpawner.h"

#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <externals/imgui/ImGuiFileDialog.h>

EnemySpawner::EnemySpawner(const std::string& name) {
	SetName(name, ObjectType::GameObject);
	worldTransform_.Initialize();
	worldTransform_.translation = { 0.0f, 0.0f, 5.0f };

}

void EnemySpawner::Update() {
	float dt = ClockManager::GetInstance()->GetDeltaTime();

	Vector3 rot = rotationDir_ * rotationSpeed_ * dt;
	worldTransform_.eulerRotation += rot;

	worldTransform_.rotationSource = RotationSource::Euler;
	worldTransform_.Update();

	spawnTimer_ += dt;
		if (spawnTimer_ >= spawnInterval_) {
			Spawn();
			spawnTimer_ = 0.0f;
		}

}

void EnemySpawner::ApplyConfig() {
	SetName(config_.name, ObjectType::GameObject);
	rotationSpeed_ = config_.rotationSpeed;
	rotationDir_ = config_.rotationDir;
	spawnInterval_ = config_.spawnInterval;
	spawnAreaMin_ = config_.spawnAreaMin;
	spawnAreaMax_ = config_.spawnAreaMax;
	maxSpawnCount_ = config_.maxSpawnCount;
}

void EnemySpawner::ExtractConfig() {
	config_.name = GetName();
	config_.rotationSpeed = rotationSpeed_;
	config_.rotationDir = rotationDir_;
	config_.spawnInterval = spawnInterval_;
	config_.spawnAreaMin = spawnAreaMin_;
	config_.spawnAreaMax = spawnAreaMax_;
	config_.maxSpawnCount = maxSpawnCount_;
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

		if (ImGui::Button(("Load Config##" + GetName()).c_str())) {
			IGFD::FileDialogConfig config;
			config.path = "Resources/Assets/Configs/";
			ImGuiFileDialog::Instance()->OpenDialog(
				("LoadDialog##" + GetName()).c_str(),
				"Load Spawner Config",
				".json",
				config
			);
		}

		if (ImGui::Button(("Save Config##" + GetName()).c_str())) {
			IGFD::FileDialogConfig config;
			config.path = "Resources/Assets/Configs/";
			ImGuiFileDialog::Instance()->OpenDialog(
				("SaveDialog##" + GetName()).c_str(),
				"Save Spawner Config",
				".json",
				config
			);
		}

		// Load 処理
		if (ImGuiFileDialog::Instance()->Display(("LoadDialog##" + GetName()).c_str())) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				LoadConfig(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}

		// Save 処理
		if (ImGuiFileDialog::Instance()->Display(("SaveDialog##" + GetName()).c_str())) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();
				SaveConfig(filePath);
			}
			ImGuiFileDialog::Instance()->Close();
		}
#endif

		ImGui::TreePop();
	}
}


void EnemySpawner::SetSceneContext(SceneContext* context) { sceneContext_ = context; }


void EnemySpawner::SetPlayerTransform(WorldTransform* playerTransform) {
	playerTransform_ = playerTransform;
	worldTransform_.parent = playerTransform->parent;
}

void EnemySpawner::Spawn() {
	if (!sceneContext_ || !ownerCollection_) return;

	// 現在の有効な敵の数を数える
	size_t aliveCount = 0;
	for (auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end();) {
		if (!(*it) || !(*it)->GetIsAlive()) {
			it = spawnedEnemies_.erase(it);
		} else {
			++it;
			++aliveCount;
		}
	}

	// 最大数に達していたらスポーンしない
	if (aliveCount >= maxSpawnCount_) return;

	// 新規スポーン
	Enemy* enemy = sceneContext_->GetObjectLibrary()->CreateAndAddObject<Enemy>("ghost.obj", "enemy");
	if (!enemy) return;

	Vector3 localOffset = Random::GenerateVector3(spawnAreaMin_, spawnAreaMax_);
	enemy->SetPosition(localOffset);
	enemy->SetParent(&worldTransform_);

	ownerCollection_->AddEnemy(enemy);
	spawnedEnemies_.push_back(enemy);
}