#pragma once

struct Vector3;

class BaseShootingController{
public:
	BaseShootingController() = default;
	virtual ~BaseShootingController() = default;

	virtual  void Update(float dt);
	virtual  void RequestShoot(const Vector3& pos, const Vector3& dir) = 0;
	virtual float GetInterval() const = 0;

	void SetCooldown(float cooldown);
	float GetCooldown()const;
protected:
	float shootCooldown_ = 0.0f;
};
