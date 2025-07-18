#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Scene/Context/SceneContext.h>

// game
#include <Game/3dObject/Actor/Bullet/BaseBullet.h>
#include <Game/3dObject/Actor/Bullet/Details/BulletDetails.h>

// c++
#include <list>
#include <memory>
#include <unordered_map>

enum class BulletType{
	Player,
	Enemy,
	Count
};

/* ========================================================================
/* bullet コンテナ
/* ===================================================================== */
class BulletContainer
	: public SceneObject{
public:
	//===================================================================*/
	//					public functions
	//===================================================================*/
	BulletContainer(const std::string& name);

	//--------- mainfunc -----------------------------------------------------
	void Update();
	void AddBullet(BulletID id, const Vector3& pos, const Vector3& vel);
	void RemoveBullet(const std::shared_ptr<BaseBullet>& bullet);
	
	//--------- ui / gui -----------------------------------------------------
	void ShowGui();
	void DerivativeGui();

	//--------- accessor -----------------------------------------------------
	const std::list<std::shared_ptr<BaseBullet>>& GetBullets(BulletID id) const;
	std::string_view GetTypeName() const override{ return "BulletContainer"; }

private:
	//===================================================================*/
	//					private variables
	//===================================================================*/
	std::unordered_map<BulletID,
		std::list<std::shared_ptr<BaseBullet>>> typedBullets_;


	float bulletSpeed_ = 60.0f; // 弾速
	Vector3 bulletScale_ {0.3f, 0.3f, 0.3f}; // 弾のスケール
};
