#include "BulletContainer.h"
#include <Engine/Scene/Utirity/SceneUtility.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/imgui.h>

BulletContainer::BulletContainer(const std::string& name) {
	bullets_.clear();
	SceneObject::SetName(name, ObjectType::GameObject);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::Update() {
	for (auto it = bullets_.begin(); it != bullets_.end(); ) {
		(*it)->Update();
		if (!(*it)->GetIsAlive()) {
			it = bullets_.erase(it);
		} else {
			++it;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		追加
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::AddBullet(const std::string& modelName,
								const Vector3& position,
								const Vector3& velocity) {
	std::unique_ptr<BaseBullet> bullet;

	if (sceneContext_) {
		CreateAndAddObject<BaseBullet>(sceneContext_, bullet, modelName, "bullet");
		sceneContext_->GetMeshRenderer()->Register(bullet->GetModel(),&bullet->GetWorldTransform());
	} else {
		bullet = std::make_unique<BaseBullet>(modelName,"bullet");
	}

	bullet->ShootInitialize(position, velocity);
	bullet->SetMoveSpeed(bulletSpeed_);
	bullet->SetScale(bulletScale_);
	bullets_.push_back(std::move(bullet));
}

/////////////////////////////////////////////////////////////////////////////////////////
//		削除
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::RemoveBullet(BaseBullet* bullet) {
	auto it = std::remove_if(bullets_.begin(), bullets_.end(),
							 [bullet](const std::unique_ptr<BaseBullet>& b) { return b.get() == bullet; });
	bullets_.erase(it, bullets_.end());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		gui
/////////////////////////////////////////////////////////////////////////////////////////
void BulletContainer::ShowGui() {
	ImGui::SeparatorText("bullet container");

	DerivativeGui();
}

void BulletContainer::DerivativeGui() {
	ImGui::DragFloat("bulletSpeed", &bulletSpeed_, 0.01f, 0.0f, 100.0f);
	ImGui::DragFloat3("bulletScale", &bulletScale_.x, 0.01f, 0.0f, 10.0f);
}
