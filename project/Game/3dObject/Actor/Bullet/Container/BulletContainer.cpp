#include "BulletContainer.h"
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/PlayerBullet.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/imgui.h>

BulletContainer::BulletContainer(const std::string& name) {
	SceneObject::SetName(name, ObjectType::GameObject);
}


void BulletContainer::Update(){
	auto* lib = SceneContext::Current()->GetObjectLibrary();

	for (auto& [type, bullets] : typedBullets_){
		for (auto it = bullets.begin(); it != bullets.end();){
			auto bullet = *it;
			bullet->Update();

			if (!bullet->GetIsAlive()){
				lib->RemoveObject(bullet);
				it = bullets.erase(it);
			} else{
				++it;
			}
		}
	}
}

void BulletContainer::AddBullet(BulletType type, const Vector3& pos, const Vector3& vel){
	std::shared_ptr<BaseBullet> bullet;

	if (type == BulletType::Player){
		bullet = SceneAPI::Instantiate<PlayerBullet>("debugCube.obj", "playerBullet");
		bullet->Initialize();
	}

	bullet->ShootInitialize(pos, vel);
	typedBullets_[type].push_back(bullet);
}

void BulletContainer::RemoveBullet(const std::shared_ptr<BaseBullet>& bullet) {
    for (auto& [type, bullets] : typedBullets_) {
        bullets.remove(bullet);
    }
}

const std::list<std::shared_ptr<BaseBullet>>& BulletContainer::GetBullets(BulletType type) const {
    static const std::list<std::shared_ptr<BaseBullet>> empty;
    auto it = typedBullets_.find(type);
    return (it != typedBullets_.end()) ? it->second : empty;
}

void BulletContainer::ShowGui() {
	ImGui::SeparatorText("bullet container");
	DerivativeGui();

	for (const auto& [type, bullets] : typedBullets_) {
		ImGui::Text("Type %d : %d bullets", static_cast<int>(type), static_cast<int>(bullets.size()));
	}
}

void BulletContainer::DerivativeGui() {
}
