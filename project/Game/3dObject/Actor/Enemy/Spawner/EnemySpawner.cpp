#include "EnemySpawner.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineJson.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <Game/3dObject/Actor/Enemy/Directory/IEnemyDirectory.h>
#include <Game/Event/Spawn/EnemySpawnEvent.h>
#include <Game/Installer/Enemy/EnemyInstaller.h>

#include <cmath>
#include <externals/imgui/imgui.h>

/////////////////////////////////////////////////////////////////////////////////////////////
///		 ctor
/////////////////////////////////////////////////////////////////////////////////////////////
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

	// Formation デフォルト設定
	formationConfig_.useFormation = true;
	formationConfig_.motionType	  = EnemyFormationMotionType::Snake;
	formationConfig_.baseZ		  = 40.0f;
	formationConfig_.speedZ		  = 0.0f;
	formationConfig_.snakeAmpX	  = 25.0f;
	formationConfig_.snakeAmpY	  = 10.0f;
	formationConfig_.snakeFreqX	  = 2.0f;
	formationConfig_.snakeFreqY	  = 1.7f;
	formationConfig_.radius		  = 30.0f;
	formationConfig_.angularSpeed = 1.2f;
	formationConfig_.dissolveTime = 0.0f;
}

/////////////////////////////////////////////////////////////////////////////////////////////
///		 初期化
/////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::Initialize() {
	// 個別の調節パラメータ適用
	const std::string configRoot = "GameObject/";
	config_.LoadConfig(configRoot + GetName());

	formation_ = std::make_unique<EnemyFormationController>();
	formation_->Initialize(formationConfig_);
	formationTimer_ = 0.0f;
}

/////////////////////////////////////////////////////////////////////////////////////////////
///		 更新
/////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::Update(float dt) {
	worldTransform_.Update();
	GarbageCollectDead();

	UpdateProximity();

	// イベントが親にある場合は、自身では更新しない
	if(dynamic_cast<EnemySpawnEvent*>(SceneObject::GetParent().get())) {
		return;
	}

	if(isActive_) {
		TickSpawnTimer(dt);
	}

	if(isActive_) {
		TickSpawnTimer(dt);

		// Formation 更新
		if(formationConfig_.useFormation && formation_) {
			formation_->Update(dt);
			formationTimer_ = formation_->GetTime();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///		 死亡敵の掃除
/////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::DissolveFormation() {

	DissolvePattern pattern = DissolvePattern::VShape;
	if(formation_) {
		pattern = formation_->GetDissolvePattern();
	}

	int index = 0;
	for(auto& e : spawnedEnemies_) {
		e->GetMovementController()->StartDissolve(index, pattern);
		++index;
	}

	formation_.reset();
	formationTimer_ = 0.0f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///		 スポーンタイマー更新
//////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::TickSpawnTimer(float dt) {
	const float d = (playerTransform_)
						? Distance_(worldTransform_.GetWorldPosition(), playerTransform_->GetWorldPosition(), useXZDistance_)
						: std::numeric_limits<float>::infinity();

	const bool within = (d <= activationRadius_);

	size_t aliveCount = 0;
	for(auto& e : spawnedEnemies_) {
		if(e && e->GetIsAlive()) ++aliveCount;
	}

	if(aliveCount < maxSpawnCount_ && within) {
		spawnTimer_ += dt;
		if(spawnTimer_ >= 0.5f) {
			Spawn();
			spawnTimer_ = 0.0f;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// 		 常時更新
//////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::AlwaysUpdate([[maybe_unused]] float dt) {
	worldTransform_.Update();
	PrimitiveDrawer::GetInstance()->DrawSphere(worldTransform_.GetWorldPosition());
}

//////////////////////////////////////////////////////////////////////////////////////////////
///		 ２点間距離計算
//////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::ApplyConfig() {
	const auto& cfg = config_.GetConfig();
	if(!cfg.name.empty()) {
		SetName(cfg.name, ObjectType::GameObject);
	}
	worldTransform_.ApplyConfig(cfg.transform);
	spawnInterval_ = cfg.spawnInterval;
	maxSpawnCount_ = cfg.maxSpawnCount;

	useXZDistance_ = cfg.useXZDistance;

	if(deactivationRadius_ < activationRadius_) {
		deactivationRadius_ = activationRadius_ + 10.0f;
	}
	formationConfig_ = cfg.formation;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// 	 設定抽出
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::ExtractConfig() {
	auto& cfg		  = config_.GetConfig();
	cfg.name		  = GetName();
	cfg.spawnInterval = spawnInterval_;
	cfg.maxSpawnCount = maxSpawnCount_;
	cfg.transform	  = worldTransform_.ExtractConfig();

	cfg.formation = formationConfig_;
	// 新規
	cfg.useXZDistance = useXZDistance_;
}

//////////////////////////////////////////////////////////////////////////////////////////////
/// 		 GUI表示
//////////////////////////////////////////////////////////////////////////////////////////////
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

	ImGui::SeparatorText("Spawner Config");
	config_.ShowGui("GameObject/" + GetName());

	// ======================================================================
	// Formation GUI
	// ======================================================================
	ImGui::SeparatorText("Formation");

	// --- Use Formation Toggle ---
	bool use = formationConfig_.useFormation;
	if(ImGui::Checkbox("Use Formation", &use)) {
		formationConfig_.useFormation = use;

		// OFF → ON で必ず再生成
		if(formationConfig_.useFormation && !formation_) {
			formation_ = std::make_unique<EnemyFormationController>();
			formation_->Initialize(formationConfig_);
			formationTimer_ = 0.0f;
		}
	}

	if(!formationConfig_.useFormation) {
		return;
	}

	// ---------- Motion Type ----------
	static const char* motionNames[] = {
		"Straight",
		"Snake",
		"Circle"};

	int motion = static_cast<int>(formationConfig_.motionType);
	if(ImGui::Combo("Motion Type", &motion, motionNames, IM_ARRAYSIZE(motionNames))) {
		formationConfig_.motionType =
			static_cast<EnemyFormationMotionType>(motion);
	}

	// ---------- Common ----------
	ImGui::DragFloat("Base Z", &formationConfig_.baseZ, 1.0f);
	ImGui::DragFloat("Speed Z", &formationConfig_.speedZ, 0.5f);

	// ---------- Circle ----------
	if(formationConfig_.motionType == EnemyFormationMotionType::Circle) {
		ImGui::DragFloat("Radius", &formationConfig_.radius, 1.0f, 0.0f, 200.0f);
		ImGui::DragFloat("Angular Speed", &formationConfig_.angularSpeed, 0.05f);
	}

	// ---------- Snake ----------
	if(formationConfig_.motionType == EnemyFormationMotionType::Snake) {
		ImGui::DragFloat("Snake Amp X", &formationConfig_.snakeAmpX, 1.0f);
		ImGui::DragFloat("Snake Amp Y", &formationConfig_.snakeAmpY, 1.0f);
		ImGui::DragFloat("Snake Freq X", &formationConfig_.snakeFreqX, 0.1f);
		ImGui::DragFloat("Snake Freq Y", &formationConfig_.snakeFreqY, 0.1f);
	}

	// ---------- Dissolve ----------
	ImGui::DragFloat("Dissolve Time",
					 &formationConfig_.dissolveTime, 0.1f, 0.0f, 10.0f);

	static const char* dissolveNames[] = {
		"Alt Left Right",
		"Four Way",
		"V Shape",
		"Circle",
		"Straight Back"};

	int dissolve = static_cast<int>(formationConfig_.dissolvePattern);
	if(ImGui::Combo("Dissolve Pattern",
					&dissolve, dissolveNames, IM_ARRAYSIZE(dissolveNames))) {

		formationConfig_.dissolvePattern =
			static_cast<DissolvePattern>(dissolve);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
///		 プレイヤー Transform 設定
//////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::SetPlayerTransform(WorldTransform* playerTransform) { playerTransform_ = playerTransform; }

///////////////////////////////////////////////////////////////////////////////////////////////////
///		 敵ディレクトリ設定
///////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::SetBulletContainer(EnemyBulletContainer* bulletContainer) { bulletContainer_ = bulletContainer; }

///////////////////////////////////////////////////////////////////////////////////////////////////
///		 JSON から設定適用
///////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///		 JSON へ設定抽出
///////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<EnemySpawner*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///		経路設定
///////////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::SetRoute(const SplineData& s) {
	enemyMoveRoute_ = s;
	enemyMoveRoute_.BuildArcTable();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///		経路データをJSONから読み込み
/////////////////////////////////////////////////////////////////////////////////////////////////
bool EnemySpawner::LoadRouteFromJson(const std::string& path) {
	SplineData tmp;
	if(!SplineJson::Load(path, tmp)) { // JSON から読み込み
		return false;
	}
	SetRoute(tmp);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///		 編隊内オフセット計算
/////////////////////////////////////////////////////////////////////////////////////////////////
Vector3 EnemySpawner::CalcFormationOffset(size_t index) const {
	if(maxSpawnCount_ == 0) return {0, 0, 0};

	int center = (int)(maxSpawnCount_ - 1) / 2;
	int i	   = (int)index;

	float spacing = 18.0f;
	float x		  = (i - center) * spacing;

	// 少し V 字型に奥方向へずらす
	float z = std::abs(i - center) * 4.0f;

	return Vector3{x, 0, z};
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///		 侵入開始位置計算
/////////////////////////////////////////////////////////////////////////////////////////////////
Vector3 EnemySpawner::CalcEntranceStartPos(size_t index) const {
	float side = (index % 2 == 0) ? -1.0f : +1.0f; // 左右外
	float y	   = Random::Generate<float>(-0.3f, 0.3f);

	// Z はかなり奥から
	return Vector3(
		side * 120.0f,
		y * 40.0f,
		-200.0f);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///		 即時全スポーン
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::SpawnAllImmediate() {
	if(!formation_ && formationConfig_.useFormation) {
		formation_ = std::make_unique<EnemyFormationController>();
		formation_->Initialize(formationConfig_);
		formationTimer_ = 0.0f;
	}

	while(spawnedEnemies_.size() < maxSpawnCount_) {
		Spawn();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///		 近接起動更新
///////////////////////////////////////////////////////////////////////////////////////////////
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
	const float	  d		= Distance_(spPos, plPos, useXZDistance_);

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
			isActive_	= false;
			spawnTimer_ = 0.0f;
			DespawnAll();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///		 スポーン
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::Spawn() {
	if(!bulletContainer_) return;

	EnemyInstaller installer;
	auto		   enemy = installer.InstallEnemy(bulletContainer_);
	if(!enemy) return;

	enemy->Initialize();
	enemy->SetPlayerTransform(playerTransform_);

	spawnedEnemies_.push_back(enemy);

	if(directory_) {
		directory_->Register(enemy);
	}

	// ✅ Dissolve 中は侵入させない
	if(formation_) {
		size_t index = spawnedEnemies_.size() - 1;

		Vector3 offset	 = CalcFormationOffset(index);
		Vector3 entrance = CalcEntranceStartPos(index);

		enemy->StartEntranceToFormation(
			formation_.get(),
			offset,
			entrance);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
///		 全デスポーン
///////////////////////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////////////////////
///		 死亡敵の掃除
///////////////////////////////////////////////////////////////////////////////////////////////
void EnemySpawner::GarbageCollectDead() {
	auto* lib = SceneContext::Current()->GetObjectLibrary();
	for(auto it = spawnedEnemies_.begin(); it != spawnedEnemies_.end();) {
		auto& e = *it;
		if(!e || e->GetDeathState() == Enemy::DeathState::Dead) {
			if(e) {
				if(directory_) directory_->Unregister(e.get());
				if(lib) lib->RemoveObject(e);
			}
			it = spawnedEnemies_.erase(it);
		} else {
			++it;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////
///		 ２点間距離計算
//////////////////////////////////////////////////////////////////////////////////////////////
float EnemySpawner::Distance_(const Vector3& a, const Vector3& b, bool useXZ) {
	const float dx = a.x - b.x;
	const float dy = a.y - b.y;
	const float dz = a.z - b.z;
	if(useXZ) {
		return std::sqrt(dx * dx + dz * dz);
	} else {
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}
}

REGISTER_SCENE_OBJECT(EnemySpawner)