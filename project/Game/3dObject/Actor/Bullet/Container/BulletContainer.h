#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>


class SceneContext;

enum class BulletType {
	Player,
	Enemy,
	Count
};

/* ========================================================================
/* bullet コンテナ
/* ===================================================================== */
class BulletContainer
	: public SceneObject {
public:
	BulletContainer(const std::string& name);
	~BulletContainer() = default;

	void Update();

	void AddBullet(BulletType type,
				   const Vector3& position,
				   const Vector3& velocity);
	void RemoveBullet(BaseBullet* bullet);

	/* ui =========================================*/
	void ShowGui() override;
	virtual void DerivativeGui();

	/* config =========================================*/

	/* accessor =========================================*/
	const std::list<BaseBullet*>& GetBullets(BulletType type) const;
	void SetSceneContext(SceneContext* context) { sceneContext_ = context; }

private:
	std::unordered_map<BulletType, std::list<BaseBullet*>> typedBullets_;
	SceneContext* sceneContext_ = nullptr;

	float bulletSpeed_ = 60.0f; // 弾速
	Vector3 bulletScale_{ 0.3f, 0.3f, 0.3f }; // 弾のスケール
};