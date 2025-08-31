#include "EnemySpawner.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include <Game/Installer/Enemy/EnemyInstaller.h>
#include <Game/3dObject/Actor/Enemy/Directory/IEnemyDirectory.h>

#include <externals/imgui/imgui.h>
#include <cmath>

EnemySpawner::EnemySpawner(const std::string& name) : SceneObject() {
	SetName(name, ObjectType::GameObject);
}

void EnemySpawner::Update(float dt) {
	// Vector3 rot = rotationDir_ * rotationSpeed_ * dt;
	// worldTransform_.eulerRotation += rot;
	// worldTransform_.rotationSource = RotationSource::Euler;

	UpdateProximity_();

	if (isActive_) {
		// 現在の生存数を数える（死体は別途回収）
		size_t aliveCount = 0;
		for (auto& e : spawnedEnemies_) {
			if (e && e->GetIsAlive()) ++aliveCount;
		}
		if (aliveCount < maxSpawnCount_) {
			spawnTimer_ += dt;
			if (spawnTimer_ >= spawnInterval_) {
				Spawn();
				spawnTimer_ = 0.0f;
			}
		}
	}

	// 3) 死体回収（Scene からも除去）
	GarbageCollectDead();
}

void EnemySpawner::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();
	// デバッグ表示
	PrimitiveDrawer::GetInstance()->DrawSphere(worldTransform_.GetWorldPosition());
	// ※ 半径の可視化 API があればここで activation/deactivation を描くと便利
}

void EnemySpawner::ApplyConfig() {
	const auto& cfg = config_.GetConfig();
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

	// 新規：半径・判定平面
	useXZDistance_ = cfg.useXZDistance;

	// 安全：ヒステリシス最低確保
	if (deactivationRadius_ < activationRadius_) {
		deactivationRadius_ = activationRadius_ + 10.0f;
	}
}

void EnemySpawner::ExtractConfig() {
	auto& cfg = config_.GetConfig();
	cfg.name = GetName();
	cfg.rotationSpeed = rotationSpeed_;
	cfg.rotationDir = rotationDir_;
	cfg.spawnInterval = spawnInterval_;
	cfg.spawnAreaMin = spawnAreaMin_;
	cfg.spawnAreaMax = spawnAreaMax_;
	cfg.maxSpawnCount = maxSpawnCount_;
	cfg.transform = worldTransform_.ExtractConfig();

	// 新規
	cfg.useXZDistance = useXZDistance_;
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

	ImGui::SeparatorText("Proximity Activation");
	ImGui::Checkbox("Use XZ Distance", &useXZDistance_);
	ImGui::DragFloat("Activation Radius", &activationRadius_, 1.0f, 0.0f, 10000.0f);
	ImGui::DragFloat("Deactivation Radius", &deactivationRadius_, 1.0f, 0.0f, 10000.0f);

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

void EnemySpawner::UpdateProximity_() {
	// プレイヤー不在なら起動しない（ゲーム開始直後の暴発防止）
	if (!playerTransform_) {
		if (isActive_) {
			isActive_ = false;
			spawnTimer_ = 0.0f;
			DespawnAll_();
		}
		return;
	}

	const Vector3 spPos = worldTransform_.GetWorldPosition();
	const Vector3 plPos = playerTransform_->GetWorldPosition();
	const float d = Distance_(spPos, plPos, useXZDistance_);

	if (!isActive_) {
		// 起動：起動半径以内に入ったら
		if (d <= activationRadius_) {
			isActive_ = true;
			spawnTimer_ = 0.0f;    // 起動直後の即湧きを避ける
			//一体はスポーン
			Spawn();
		}
	} else {
		// 停止：停止半径以上に離れたら
		if (d >= deactivationRadius_) {
			isActive_ = false;
			spawnTimer_ = 0.0f;    // 非アクティブ中はタイマー蓄積しない
			DespawnAll_();         // このスポナーの敵をまとめて消す
		}
	}
}

void EnemySpawner::Spawn() {
	EnemyInstaller installer;
	auto enemy = installer.InstallEnemy();
	if (!enemy) return;

	enemy->Initialize();
	enemy->SetPlayerTransform(playerTransform_); // null でも可（後で再配線）

	// 位置と親子付け（ローカルでランダム）
	Vector3 localOffset = Random::GenerateVector3(spawnAreaMin_, spawnAreaMax_);
	enemy->SetPosition(localOffset);
	enemy->SetParent(&worldTransform_);

	// 自前リストに登録
	spawnedEnemies_.push_back(enemy);

	if (directory_) { directory_->Register(enemy); }
}

void EnemySpawner::DespawnAll_() {
	auto* lib = SceneContext::Current()->GetObjectLibrary();
	for (auto& e : spawnedEnemies_) {
		if (e) {
			if (directory_) directory_->Unregister(e.get());
			if (lib) lib->RemoveObject(e);
		}
	}
	spawnedEnemies_.clear();
}

void EnemySpawner::GarbageCollectDead() {
	auto* lib = SceneContext::Current()->GetObjectLibrary();
	for (auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end();) {
		auto& e = *it;
		if (!e || !e->GetIsAlive()) {
			if (e) {
				if (directory_) directory_->Unregister(e.get());
				if (lib) lib->RemoveObject(e);
			}
			it = spawnedEnemies_.erase(it);
		} else {
			++it;
		}
	}
}

float EnemySpawner::Distance_(const Vector3& a, const Vector3& b, bool useXZ) {
	const float dx = a.x - b.x;
	const float dy = a.y - b.y;
	const float dz = a.z - b.z;
	if (useXZ) {
		return std::sqrt(dx * dx + dz * dz);
	} else {
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}
}

REGISTER_SCENE_OBJECT(EnemySpawner)
