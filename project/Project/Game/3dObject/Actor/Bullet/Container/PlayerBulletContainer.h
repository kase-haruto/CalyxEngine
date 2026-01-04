#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/Container/BulletContainer.h>


class PlayerBulletContainer :
	public BulletContainer{
public:
	//===================================================================*/
	//						public functions
	//===================================================================*/
	PlayerBulletContainer(const std::string name);
	PlayerBulletContainer() = default;
	~PlayerBulletContainer()override = default;

	//--------- mainFunc -----------------------------------------------------
	void AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) override;
	void AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet);
	std::string_view GetTypeName() const override{ return "PlayerBulletContainer"; }
};

