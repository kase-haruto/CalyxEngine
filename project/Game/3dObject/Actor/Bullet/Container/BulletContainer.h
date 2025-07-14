#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Foundation/Math/Vector3.h>

#include <unordered_map>
#include <list>
#include <memory>

enum class BulletType{
	Player,
	Enemy,
	Count
};

/* ========================================================================
/* bullet コンテナ
/* ===================================================================== */
class BulletContainer
	: public SceneObject{
public:
	BulletContainer(const std::string& name);

	void ShowGui();
	void DerivativeGui();

	void Update();

	void AddBullet(BulletType type, const Vector3& pos, const Vector3& vel);
	void RemoveBullet(const std::shared_ptr<BaseBullet>& bullet);

	const std::list<std::shared_ptr<BaseBullet>>& GetBullets(BulletType type) const;

	void SetSceneContext(SceneContext* context){ sceneContext_ = context; }

	std::string_view GetTypeName() const override{ return "BulletContainer"; }
private:
	std::unordered_map<BulletType, std::list<std::shared_ptr<BaseBullet>>> typedBullets_;
	SceneContext* sceneContext_ = nullptr;

	float bulletSpeed_ = 60.0f; // 弾速
	Vector3 bulletScale_ {0.3f, 0.3f, 0.3f}; // 弾のスケール
};
