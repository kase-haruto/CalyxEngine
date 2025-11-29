#pragma once

struct Vector3;

class BaseShootingController{
public:
	BaseShootingController();
	virtual ~BaseShootingController();

	virtual  void Update(float dt);
	virtual  bool RequestShoot(const Vector3& pos, const Vector3& dir) = 0;
	virtual float GetInterval() const = 0;

	void SetCooldown(float cooldown);
	float GetCooldown()const;
protected:
	float shootCooldown_ = 0.0f;
};