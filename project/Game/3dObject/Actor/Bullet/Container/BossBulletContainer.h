#pragma once
#include "BulletContainer.h"

class BossBulletContainer :
    public BulletContainer {
public:
	//===================================================================*/
	//		public method
	//===================================================================*/
	BossBulletContainer(const std::string& name);
	BossBulletContainer() = delete;
	~BossBulletContainer() override = default;

public:
	void Update(float dt);
	void AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) override;
	std::string_view GetTypeName() const override { return "BossBulletContainer"; }
};

