#pragma once
#include "BulletContainer.h"

class BossBulletContainer :
    public BulletContainer {
public:
	//===================================================================*/
	//		public method
	//===================================================================*/
	BossBulletContainer(const std::string& name);
	~BossBulletContainer() override;

public:
	void Update(float dt);
	void AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) override;
	void AddBullet(BulletID id, const std::shared_ptr<BaseBullet>& bullet);
	std::string_view GetTypeName() const override { return "BossBulletContainer"; }
};

