#pragma once

namespace CalyxMath {
	struct Vector3;
} 

class IBulletShooter{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	virtual ~IBulletShooter() = default;
	virtual void Shoot(const CalyxMath::Vector3& origin, const CalyxMath::Vector3& direction) = 0;
};

