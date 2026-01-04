#pragma once

#include "BulletContainer.h"

class EnemyBulletContainer
	: public BulletContainer{
public:
	//===================================================================*/
	//		public method
	//===================================================================*/
	EnemyBulletContainer(const std::string& name);
	EnemyBulletContainer() = delete;
	~EnemyBulletContainer() override = default;

public:
	void Update(float dt);
	void AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) override;
	void AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet);
	std::string_view GetTypeName() const override{ return "EnemyBulletContainer"; }
};
