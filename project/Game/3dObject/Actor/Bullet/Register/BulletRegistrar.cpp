#include "BulletRegistrar.h"
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>
#include <Game/3dObject/Actor/Bullet/Factory/BulletFactory.h>
#include <Game/3dObject/Actor/Bullet/PlayerBullet/PlayerBullet.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		使用する弾の登録
/////////////////////////////////////////////////////////////////////////////////////////
namespace BulletRegistrar{
	/////////////////////////////////////////////////////////////////////////////////////////
	//		使用するすべての弾の登録
	/////////////////////////////////////////////////////////////////////////////////////////
	void RegisterAll(){
		BulletFactory::Register(BulletID::Player_Straight, []{
			auto bullet = SceneAPI::Instantiate<PlayerBullet>("debugCube.obj", "playerBullet");
			bullet->Initialize();
			return bullet;
								});
	}
}