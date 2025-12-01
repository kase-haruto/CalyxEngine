#include "BulletRegistrar.h"
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include "Game/3dObject/Actor/Bullet/BossBullet/BossHomingBullet.h"
#include <Game/3dObject/Actor/Bullet/BossBullet/BossBullet.h>
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>
#include <Game/3dObject/Actor/Bullet/EnemyBullet/EnemyBullet.h>
#include <Game/3dObject/Actor/Bullet/EnemyBullet/EnemyHomingBullet.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/HomingBullet.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/PlayerBullet.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		使用する弾の登録
/////////////////////////////////////////////////////////////////////////////////////////
namespace BulletRegistrar {
void RegisterAll() {

	//===================================================================*/
	//						player通常弾
	//===================================================================*/
	BulletFactory::Register(BulletID::Player_Straight, [] {
		auto bullet = SceneAPI::Instantiate<PlayerBullet>("debugCube.obj", "playerBullet");
		bullet->Initialize();
		return bullet;
	});

	//===================================================================*/
	//						enemy通常弾
	//===================================================================*/
	BulletFactory::Register(BulletID::Enemy_Straight, [] {
		auto bullet = SceneAPI::Instantiate<EnemyBullet>("enemyBullet.obj", "enemyBullet");
		bullet->Initialize();
		return bullet;
	});

	//===================================================================*/
	//						enemyホーミング弾
	//===================================================================*/
	BulletFactory::Register(BulletID::Enemy_Homing, [] {
		auto bullet = SceneAPI::Instantiate<EnemyHomingBullet>("enemyBullet.obj", "enemyBullet");
		bullet->Initialize();
		return bullet;
	});

	//===================================================================*/
	//						追尾弾
	//===================================================================*/
	BulletFactory::Register(BulletID::Player_Homing, [] {
		auto bullet = SceneAPI::Instantiate<HomingBullet>("debugCube.obj", "homingBullet");
		bullet->Initialize();
		return bullet;
	});

	//===================================================================*/
	//						ボス弾
	//===================================================================*/
	BulletFactory::Register(BulletID::Boss_Straight, [] {
		auto bullet = SceneAPI::Instantiate<BossBullet>("debugCube.obj", "bossBullet");
		bullet->Initialize();
		return bullet;
	});

	//===================================================================*/
	//						ボスホーミング弾
	//===================================================================*/
	BulletFactory::Register(BulletID::Boss_Homing, [] {
		auto bullet = SceneAPI::Instantiate<BossHomingBullet>("debugCube.obj", "bossHomingBullet");
		bullet->Initialize();
		return bullet;
	});
}
} // namespace BulletRegistrar