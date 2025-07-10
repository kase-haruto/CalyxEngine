#include "EnemyCollection.h"

#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utirity/SceneUtility.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Game/3dObject/Actor/Enemy/Spawner/EnemySpawner.h>

#include <externals/imgui/imgui.h>

EnemyCollection::EnemyCollection(const std::string& name) {
	SetName(name, ObjectType::GameObject);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void EnemyCollection::Update(){
	// スポナー更新
	for (auto& spawner : spawners_){
		spawner->Update();
	}

	auto* sceneLibrary = sceneContext_ ? sceneContext_->GetObjectLibrary() : nullptr;
	if (!sceneLibrary) return;

	for (auto it = enemies_.begin(); it != enemies_.end(); ){
		auto& enemy = *it;
		enemy->Update();

		if (!enemy->GetIsAlive()){
			sceneLibrary->RemoveObject(enemy);  // ✅ shared_ptr で参照カウント一致
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


void EnemyCollection::SetSceneContext(SceneContext* context){
	sceneContext_ = context;
	for (auto& spawner : spawners_){
		spawner->SetSceneContext(context);
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
		spawner->SetSceneContext(sceneContext_);
		spawner->SetPlayerTransform(playerTransform_);
		spawner->SetOwner(this);
		spawners_.emplace_back(spawner);
	}
}

void EnemyCollection::CreateSpawners(){
	// 左回り
	auto leftSpawner = std::make_shared<EnemySpawner>("leftSpawner");
	sceneContext_->AddEditorObject(leftSpawner);

	leftSpawner->SetRotationSpeed(0.4f);
	leftSpawner->SetRotationDir({0, 1, 0});
	leftSpawner->SetSpawnArea({-10, 0, -15}, {10, 5, -20});
	AddSpawner(leftSpawner);

	// 右回り
	auto rightSpawner = std::make_shared<EnemySpawner>("rightSpawner");
	sceneContext_->AddEditorObject(rightSpawner);

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
