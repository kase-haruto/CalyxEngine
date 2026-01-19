#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Engine/Application/Effects/FxObject.h>
#include <Engine/Foundation/Serialization/SerializableObject.h>

class HomingBullet :
	public BaseBullet{
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	HomingBullet() = default;
	HomingBullet(const std::string& modelName, const std::string& name);
	~HomingBullet();
	void ShootInitialize(const CalyxMath::Vector3& initPos, const CalyxMath::Vector3& velocity)override;
	void Initialize()override;
	void OnShot();
	void SetTarget(const Actor* target);
	void Update(float dt) override;

	const CalyxMath::Vector3 GetCenterPos() const override;

private:
	//===================================================================*/
	//			Inner Class
	//===================================================================*/
	struct BulletParam : public CalyxEngine::SerializableObject {
		BulletParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float homingSpeed = 2.0f;
		float rotateSpeed = 100.0f;
	} param_;

protected:
	const Actor* target_ = nullptr;
	float homingSpeed_ = 2.0f;
	float rotateSpeed_ = 100.0f;

	// trail
	std::weak_ptr<CalyxEffect::FxObject> trailFx_;
};