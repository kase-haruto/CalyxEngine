#include "EnemySpawner.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Game/3dObject/Actor/Enemy/Collection/EnemyCollection.h>
#include <Game/Installer/Enemy/EnemyInstaller.h>

EnemySpawner::EnemySpawner(const std::string& name){
	SetName(name,ObjectType::GameObject);
	worldTransform_.Initialize();
	worldTransform_.translation = {0.0f, 0.0f, 5.0f};

	auto ctx = SceneContext::Current();
	//worldTransform_.parent = &ctx->GetCameraMgr()->GetMain3d()->GetWorldTransform();
}

void EnemySpawner::Update(float dt){
	Vector3 rot = rotationDir_ * rotationSpeed_ * dt;
	worldTransform_.eulerRotation += rot;

	worldTransform_.rotationSource = RotationSource::Euler;

	spawnTimer_ += dt;
	if (spawnTimer_ >= spawnInterval_){
		Spawn();
		spawnTimer_ = 0.0f;
	}
}

void EnemySpawner::AlwaysUpdate([[maybe_unused]] float dt){
	worldTransform_.Update();

	PrimitiveDrawer::GetInstance()->DrawSphere(worldTransform_.GetWorldPosition());
}

void EnemySpawner::ApplyConfig(){
	auto cfg = config_.GetConfig();

	if (!cfg.name.empty()) {
		SetName(cfg.name, ObjectType::GameObject);
	}
	rotationSpeed_ = cfg.rotationSpeed;
	rotationDir_ = cfg.rotationDir;
	spawnInterval_ = cfg.spawnInterval;
	spawnAreaMin_ = cfg.spawnAreaMin;
	spawnAreaMax_ = cfg.spawnAreaMax;
	maxSpawnCount_ = cfg.maxSpawnCount;
}

void EnemySpawner::ExtractConfig(){
	auto cfg = config_.GetConfig();
	cfg.name = GetName();
	cfg.rotationSpeed = rotationSpeed_;
	cfg.rotationDir = rotationDir_;
	cfg.spawnInterval = spawnInterval_;
	cfg.spawnAreaMin = spawnAreaMin_;
	cfg.spawnAreaMax = spawnAreaMax_;
	cfg.maxSpawnCount = maxSpawnCount_;
}

void EnemySpawner::ShowGui(){
	// Transform情報
	worldTransform_.ShowImGui();

	ImGui::DragFloat("Rotation Speed",&rotationSpeed_,0.1f);
	ImGui::DragFloat3("Rotation Dir",&rotationDir_.x,0.1f);
	ImGui::InputFloat("Spawn Interval",&spawnInterval_);
	ImGui::DragInt("Max Spawn Count",reinterpret_cast<int*>(&maxSpawnCount_),1,1,100);
	ImGui::DragFloat3("Spawn Area Min",&spawnAreaMin_.x,0.1f);
	ImGui::DragFloat3("Spawn Area Max",&spawnAreaMax_.x,0.1f);

#ifdef _DEBUG
	ImGui::SeparatorText("Spawner Config");
	config_.ShowGui();
#endif
}


void EnemySpawner::SetPlayerTransform(WorldTransform* playerTransform){
	playerTransform_ = playerTransform;
	//worldTransform_.parent = playerTransform->parent;
}

void EnemySpawner::ApplyConfigFromJson(const nlohmann::json& j){
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void EnemySpawner::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast<EnemySpawner*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

void EnemySpawner::Spawn(){
	//if (!ownerCollection_) return;

	// 有効な敵だけ残す
	size_t aliveCount = 0;
	for (auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end();){
		if (!(*it) || !(*it)->GetIsAlive()){ it = spawnedEnemies_.erase(it); }
		else{
			++it;
			++aliveCount;
		}
	}

	if (aliveCount >= maxSpawnCount_) return;

	// 新規スポーン
	EnemyInstaller installer;
	auto enemy = installer.InstallEnemy();
	enemy->Initialize();
	enemy->SetPlayerTransform(playerTransform_);
	if (!enemy) return;

	Vector3 localOffset = Random::GenerateVector3(spawnAreaMin_,spawnAreaMax_);
	enemy->SetPosition(localOffset);
	enemy->SetParent(&worldTransform_);

	//ownerCollection_->AddEnemy(enemy);
	spawnedEnemies_.push_back(enemy);
}

REGISTER_SCENE_OBJECT(EnemySpawner)
