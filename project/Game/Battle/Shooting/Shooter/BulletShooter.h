#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Objects/Transform/Transform.h>

#include <optional>
#include <memory>

class BulletShooter{
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	~BulletShooter() = default;

	virtual void Shoot(class BulletContainer*,
					   const struct Vector3& origin,
					   const struct Vector3& dir,
					   const std::optional<WorldTransform> & = std::nullopt) = 0;

private:

};
