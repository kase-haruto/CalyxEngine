#pragma once

namespace CxMath {
	struct Vector3;
} 

class IBulletShooter{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	virtual ~IBulletShooter() = default;
	virtual void Shoot(const CxMath::Vector3& origin, const CxMath::Vector3& direction) = 0;
};

