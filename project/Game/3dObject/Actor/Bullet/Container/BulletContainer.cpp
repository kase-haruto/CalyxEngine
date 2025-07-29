#include "BulletContainer.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/3dObject/Actor/Bullet/PlayerBullet/PlayerBullet.h>

// externals
#include <externals/imgui/imgui.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
BulletContainer::BulletContainer(const std::string& name){
	SceneObject::SetName(name, ObjectType::GameObject);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::Update(float dt){
	auto* lib = SceneContext::Current()->GetObjectLibrary();

	for (auto& [id, bullets] : typedBullets_){
		for (auto it = bullets.begin(); it != bullets.end(); ){
			auto bullet = *it;
			bullet->Update(dt);

			if (!bullet->GetIsAlive()){
				lib->RemoveObject(bullet);
				it = bullets.erase(it);
			} else{
				++it;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		削除
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::RemoveBullet(const std::shared_ptr<BaseBullet>& bullet){
	for (auto& [type, bullets] : typedBullets_){
		bullets.remove(bullet);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		弾の取得
/////////////////////////////////////////////////////////////////////////////////////////
const std::list<std::shared_ptr<BaseBullet>>& BulletContainer::GetBullets(BulletID id) const{
	static const std::list<std::shared_ptr<BaseBullet>> empty;
	auto it = typedBullets_.find(id);
	return (it != typedBullets_.end()) ? it->second : empty;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ui
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::ShowGui(){
	ImGui::SeparatorText("bullet container");
	DerivativeGui();

	for (const auto& [type, bullets] : typedBullets_){
		ImGui::Text("Type %d : %d bullets", static_cast< int >(type), static_cast< int >(bullets.size()));
	}
}

void BulletContainer::DerivativeGui(){
	editBullet_->ShowGui();
}
