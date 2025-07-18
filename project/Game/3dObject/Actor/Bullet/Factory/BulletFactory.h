#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>

// c++
#include <functional>
#include <memory>
#include <unordered_map>

class BulletFactory{
public:
	//===================================================================*/
	//				public functions
	//===================================================================*/
	using BulletCreator = std::function<std::shared_ptr<BaseBullet>()>;

	static void Register(BulletID id, BulletCreator creator);
	static std::shared_ptr<BaseBullet> Create(BulletID id);

private:
	//===================================================================*/
	//				private functions
	//===================================================================*/
	static std::unordered_map<BulletID, BulletCreator> registry_;
};