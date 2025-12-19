#pragma once
#include <Engine/Foundation/Math/Quaternion.h>

class PlayerDodgeSystem;
class WorldTransform;

class PlayerDodgeSpinMotion {
public:
	void Initialize(PlayerDodgeSystem* dodge, WorldTransform* wt);

	void OnDodgeStart();
	void OnDodgeEnd();
	void Update(float dt);
	void OnPerfect();

private:
	PlayerDodgeSystem* dodge_ = nullptr;
	WorldTransform*    wt_    = nullptr;

	CalyxMath::Quaternion baseRot_{0,0,0,1};
};
