#include "EnemyCollection.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include <externals/imgui/imgui.h>

EnemyCollection::EnemyCollection(const std::string& name){ SetName(name,ObjectType::GameObject); }

void EnemyCollection::Initialize(){
	worldTransform_.Initialize();
	//CreateSpawners();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::Update(float dt){
	// スポナー更新
	for (auto& spawner : spawners_){ spawner->Update(dt); }

	auto* lib = SceneContext::Current()->GetObjectLibrary();
	if (!lib) return;

	for (auto it = enemies_.begin(); it != enemies_.end();){
		std::shared_ptr<Enemy>& enemy = *it;
		if (!enemy->GetIsAlive()){
			lib->RemoveObject(enemy);
			it = enemies_.erase(it);
			deadEnemyCount++;
		}
		else{ ++it; }
	}
}

void EnemyCollection::AlwaysUpdate([[maybe_unused]] float dt){ worldTransform_.Update(); }

void EnemyCollection::ShowGui(){
	/*ImGui::Text("Enemy Count : %d", static_cast< int >(enemies_.size()));
	ImGui::SeparatorText("Spawners");

	int idx = 0;
	for (auto& spawner : spawners_){
		ImGui::PushID(idx);
		spawner->ShowGui();
		ImGui::PopID();
		++idx;
	}*/
}

void EnemyCollection::SetPlayerTransform(WorldTransform* pTransform){ playerTransform = pTransform; }

void EnemyCollection::AddEnemy(const std::shared_ptr<Enemy>& enemy){ enemies_.push_back(enemy); }

///////////////////////////////////////////////////////////////////////////////////////////
//		追加
///////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::AddSpawner(const std::shared_ptr<EnemySpawner>& spawner){
	if (spawner){
		//spawner->SetOwner(this);
		spawner->SetPlayerTransform(playerTransform);
		spawners_.emplace_back(spawner);
	}
}

void EnemyCollection::CreateSpawners(){
	auto self = shared_from_this();
	// 左回り
	std::shared_ptr<EnemySpawner> leftSpawner;
	leftSpawner = SceneAPI::Instantiate<EnemySpawner>("leftSpawner");

	leftSpawner->SetRotationSpeed(0.4f);
	leftSpawner->SetRotationDir({0, 1, 0});
	leftSpawner->SetSpawnArea({-10, 0, -15},{10, 5, -20});
	AddSpawner(leftSpawner);

	// 右回り
	std::shared_ptr<EnemySpawner> rightSpawner;
	rightSpawner = SceneAPI::Instantiate<EnemySpawner>("rightSpawner");

	rightSpawner->SetRotationSpeed(-0.6f);
	rightSpawner->SetRotationDir({0, 1, 0});
	rightSpawner->SetSpawnArea({-15, 0, -30},{15, 7, -20});
	AddSpawner(rightSpawner);
}


/////////////////////////////////////////////////////////////////////////////////////////
//		クリア
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::Clear(){
	enemies_.clear();
	spawners_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::ApplyConfig(){
	const auto& cfg = config_.GetConfig();

	name_ = cfg.name;
	id_ = cfg.guid;
	parentId_ = cfg.parentGuid;
}

void EnemyCollection::ExtractConfig(){
	auto& cfg = config_.GetConfig();
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name = name_;
	cfg.guid = id_;
	cfg.parentGuid = parentId_;
}

void EnemyCollection::ApplyConfigFromJson(const nlohmann::json& j){
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void EnemyCollection::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast<EnemyCollection*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}


REGISTER_SCENE_OBJECT(EnemyCollection)
