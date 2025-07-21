#pragma once

struct Vector3;

class IBulletShooter{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	virtual ~IBulletShooter() = default;
	virtual void Shoot(const Vector3& origin, const Vector3& direction) = 0;
};

