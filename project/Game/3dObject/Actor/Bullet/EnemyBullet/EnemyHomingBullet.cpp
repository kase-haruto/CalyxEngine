#include "EnemyHomingBullet.h"

#include <Engine/Scene/Utility/SceneUtility.h>
EnemyHomingBullet::EnemyHomingBullet(const std::string& modelName, const std::string& name)
	: BaseEnemyHomingBullet::BaseEnemyHomingBullet(modelName, name) {
	trailFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("TrailFx");
	if (auto fx = trailFx_.lock()) {
		fx->LoadFromPath("Effect/EnemyBulletTrail");
	}
}

EnemyHomingBullet::~EnemyHomingBullet() = default;
