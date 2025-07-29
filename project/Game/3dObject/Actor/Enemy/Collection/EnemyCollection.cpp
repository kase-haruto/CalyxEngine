#include "EnemyCollection.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>
#include <Engine/Scene/Utility/SceneUtility.h>

#include <externals/imgui/imgui.h>

EnemyCollection::EnemyCollection(const std::string& name) {
	SetName(name, ObjectType::GameObject);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::Update(float dt){
	// スポナー更新
	for (auto& spawner : spawners_){
		spawner->Update(dt);
	}

	auto* lib = SceneContext::Current()->GetObjectLibrary();
	if (!lib) return;

	for (auto it = enemies_.begin(); it != enemies_.end(); ){
		auto& enemy = *it;
		enemy->Update(dt);

		if (!enemy->GetIsAlive()){
			lib->RemoveObject(enemy);
			it = enemies_.erase(it);
			deadEnemyCount++;
		} else{
			++it;
		}
	}
}

void EnemyCollection::ShowGui(){
	ImGui::Text("Enemy Count : %d", static_cast< int >(enemies_.size()));
	ImGui::SeparatorText("Spawners");

	int idx = 0;
	for (auto& spawner : spawners_){
		ImGui::PushID(idx);
		spawner->ShowGui();
		ImGui::PopID();
		++idx;
	}
}



void EnemyCollection::SetPlayerTransform(WorldTransform* pTransform) {
		playerTransform_ = pTransform;
}

void EnemyCollection::AddEnemy(const std::shared_ptr<Enemy>& enemy){
	enemies_.push_back(enemy);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		追加
///////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::AddSpawner(const std::shared_ptr<EnemySpawner>& spawner){
	if (spawner){
		spawner->SetPlayerTransform(playerTransform_);
		spawner->SetOwner(this);
		spawners_.emplace_back(spawner);
	}
}

void EnemyCollection::CreateSpawners(){
	// 左回り
	std::shared_ptr<EnemySpawner> leftSpawner;
	leftSpawner = SceneAPI::Instantiate<EnemySpawner>("leftSpawner");

	leftSpawner->SetRotationSpeed(0.4f);
	leftSpawner->SetRotationDir({0, 1, 0});
	leftSpawner->SetSpawnArea({-10, 0, -15}, {10, 5, -20});
	AddSpawner(leftSpawner);

	// 右回り
	std::shared_ptr<EnemySpawner> rightSpawner;
	rightSpawner = SceneAPI::Instantiate<EnemySpawner>("rightSpawner");

	rightSpawner->SetRotationSpeed(-0.6f);
	rightSpawner->SetRotationDir({0, 1, 0});
	rightSpawner->SetSpawnArea({-15, 0, -30}, {15, 7, -20});
	AddSpawner(rightSpawner);
}



/////////////////////////////////////////////////////////////////////////////////////////
//		クリア
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::Clear() {
	enemies_.clear();
	spawners_.clear();
}
