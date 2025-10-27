#include "EnemySpawner.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineJson.h>

#include <Game/Installer/Enemy/EnemyInstaller.h>
#include <Game/3dObject/Actor/Enemy/Directory/IEnemyDirectory.h>


#include <externals/imgui/imgui.h>
#include <cmath>

EnemySpawner::EnemySpawner(const std::string& name) : SceneObject() {
	SetName(name, ObjectType::GameObject);

	// 発生的の移動ルート読み込み
	if(!LoadRouteFromJson(moveRoutePath_)) {
		enemyMoveRoute_		   = SplineData{};
		enemyMoveRoute_.closed = false;
		enemyMoveRoute_.BuildArcTable();
	}

	config_.SetOnApplied([this](const EnemySpawnerConfig&) {
		this->ApplyConfig();
	});

	config_.SetOnExtracted([this](const EnemySpawnerConfig&) {
		this->ExtractConfig();
	});

	
}

void EnemySpawner::Initialize() {
	// 個別の調節パラメータ適用
	const std::string configRoot = "GameObject/";
	config_.LoadConfig(configRoot + GetName());
}

void EnemySpawner::Update(float dt) {
	// 自分の行列は先に更新（距離計算の基準になる）
	worldTransform_.Update();

	UpdateProximity();

	if(isActive_) {
		// 現在の距離を算出（XZ か 3D かはフラグに従う）
		const float d = (playerTransform_)
							? Distance_(worldTransform_.GetWorldPosition(),playerTransform_->GetWorldPosition(),useXZDistance_)
							: std::numeric_limits<float>::infinity();

		// 距離内にタイマーを進める（距離外では停止＝保持）
		const bool within = (d <= activationRadius_);

		// 現在の生存数（死体は別途回収）
		size_t aliveCount = 0;
		for(auto& e : spawnedEnemies_) { if(e && e->GetIsAlive()) ++aliveCount; }

		if(aliveCount < maxSpawnCount_) {
			if(within) {
				spawnTimer_ += dt;
				if(spawnTimer_ >= 0.5f) {
					Spawn();
					spawnTimer_ = 0.0f; // 次の湧きへ
				}
			}
		}
	}

	GarbageCollectDead();
}


void EnemySpawner::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();
	PrimitiveDrawer::GetInstance()->DrawSphere(worldTransform_.GetWorldPosition());
}

void EnemySpawner::ApplyConfig() {
	const auto& cfg = config_.GetConfig();
	if(!cfg.name.empty()) { SetName(cfg.name,ObjectType::GameObject); }
	worldTransform_.ApplyConfig(cfg.transform);
	rotationSpeed_ = cfg.rotationSpeed;
	rotationDir_   = cfg.rotationDir;
	spawnInterval_ = cfg.spawnInterval;
	spawnAreaMin_  = cfg.spawnAreaMin;
	spawnAreaMax_  = cfg.spawnAreaMax;
	maxSpawnCount_ = cfg.maxSpawnCount;

	useXZDistance_ = cfg.useXZDistance;

	if(deactivationRadius_ < activationRadius_) { deactivationRadius_ = activationRadius_ + 10.0f; }
}

void EnemySpawner::ExtractConfig() {
	auto& cfg         = config_.GetConfig();
	cfg.name          = GetName();
	cfg.rotationSpeed = rotationSpeed_;
	cfg.rotationDir   = rotationDir_;
	cfg.spawnInterval = spawnInterval_;
	cfg.spawnAreaMin  = spawnAreaMin_;
	cfg.spawnAreaMax  = spawnAreaMax_;
	cfg.maxSpawnCount = maxSpawnCount_;
	cfg.transform     = worldTransform_.ExtractConfig();

	// 新規
	cfg.useXZDistance = useXZDistance_;
}

void EnemySpawner::ShowGui() {
	// Transform情報
	worldTransform_.ShowImGui();

	ImGui::DragFloat("Rotation Speed",&rotationSpeed_,0.1f);
	ImGui::DragFloat3("Rotation Dir",&rotationDir_.x,0.1f);
	ImGui::InputFloat("Spawn Interval",&spawnInterval_);
	ImGui::DragInt("Max Spawn Count",reinterpret_cast<int*>(&maxSpawnCount_),1,1,100);
	ImGui::DragFloat3("Spawn Area Min",&spawnAreaMin_.x,0.1f);
	ImGui::DragFloat3("Spawn Area Max",&spawnAreaMax_.x,0.1f);

	ImGui::SeparatorText("Proximity Activation");
	ImGui::Checkbox("Use XZ Distance",&useXZDistance_);
	ImGui::DragFloat("Activation Radius",&activationRadius_,1.0f,0.0f,10000.0f);
	ImGui::DragFloat("Deactivation Radius",&deactivationRadius_,1.0f,0.0f,10000.0f);

	ImGui::SeparatorText("Spawner Config");
	config_.ShowGui("GameObject/"+GetName());
}

void EnemySpawner::SetPlayerTransform(WorldTransform* playerTransform) { playerTransform_ = playerTransform; }

void EnemySpawner::SetBulletContainer(EnemyBulletContainer* bulletContainer) { bulletContainer_ = bulletContainer; }

void EnemySpawner::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void EnemySpawner::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<EnemySpawner*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

void EnemySpawner::SetRoute(const SplineData& s) {
	enemyMoveRoute_ = s;
	enemyMoveRoute_.BuildArcTable();
}

bool EnemySpawner::LoadRouteFromJson(const std::string& path) {
	SplineData tmp;
	if(!SplineJson::Load(path,tmp)) { // JSON から読み込み
		return false;
	}
	SetRoute(tmp);
	return true;
}

void EnemySpawner::UpdateProximity() {
	// プレイヤー不在なら停止＆掃除
	if(!playerTransform_) {
		if(isActive_) {
			isActive_ = false;
			// spawnTimer_ は保持しても良いが、明示的に止めたいなら 0 にする
			spawnTimer_ = 0.0f;
			DespawnAll();
		}
		return;
	}

	const Vector3 spPos = worldTransform_.GetWorldPosition();
	const Vector3 plPos = playerTransform_->GetWorldPosition();
	const float   d     = Distance_(spPos,plPos,useXZDistance_);

	if(!isActive_) {
		// 起動：起動半径以内に入ったら
		if(d <= activationRadius_) {
			isActive_ = true;
			// タイマーはゼロから積み上げ開始
			spawnTimer_ = 0.0f;
		}
	} else {
		// 停止：停止半径以上で停止＆全消去
		if(d >= deactivationRadius_) {
			isActive_   = false;
			spawnTimer_ = 0.0f;
			DespawnAll();
		}
	}
}

void EnemySpawner::Spawn() {
	if(!bulletContainer_) return; // 敵弾コンテナがなければ早期return

	EnemyInstaller installer;
	auto           enemy = installer.InstallEnemy(bulletContainer_);
	if(!enemy) return;

	enemy->Initialize();
	enemy->SetPlayerTransform(playerTransform_);
	enemy->SetRouteSpline(enemyMoveRoute_);

	// 位置と親子付け（ローカルでランダム）
	enemy->SetSpawnerAnchor(&worldTransform_);

	// 自前リストに登録
	spawnedEnemies_.push_back(enemy);

	if(directory_) { directory_->Register(enemy); }
}

void EnemySpawner::DespawnAll() {
	auto* lib = SceneContext::Current()->GetObjectLibrary();
	for(auto& e : spawnedEnemies_) {
		if(e) {
			if(directory_) directory_->Unregister(e.get());
			if(lib) lib->RemoveObject(e);
		}
	}
	spawnedEnemies_.clear();
}

void EnemySpawner::GarbageCollectDead() {
	auto* lib = SceneContext::Current()->GetObjectLibrary();
	for(auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end();) {
		auto& e = *it;
		if(!e || !e->GetIsAlive()) {
			if(e) {
				if(directory_) directory_->Unregister(e.get());
				if(lib) lib->RemoveObject(e);
			}
			it = spawnedEnemies_.erase(it);
		} else { ++it; }
	}
}

float EnemySpawner::Distance_(const Vector3& a,const Vector3& b,bool useXZ) {
	const float dx = a.x - b.x;
	const float dy = a.y - b.y;
	const float dz = a.z - b.z;
	if(useXZ) { return std::sqrt(dx * dx + dz * dz); } else { return std::sqrt(dx * dx + dy * dy + dz * dz); }
}

REGISTER_SCENE_OBJECT(EnemySpawner)