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
	void AddBullet(BulletID id, const CxMath::Vector3& pos, const CxMath::Vector3& vel) override;
	void AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet);
	std::string_view GetTypeName() const override{ return "EnemyBulletContainer"; }
};