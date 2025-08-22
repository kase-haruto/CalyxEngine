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
	virtual ~BulletContainer() = default;

	//--------- mainfunc -----------------------------------------------------
	virtual void AddBullet(BulletID id, const Vector3& pos, const Vector3& vel) = 0;
	virtual void RemoveBullet(const std::shared_ptr<BaseBullet>& bullet);
	void Update(float dt)override;
	
	//--------- ui / gui -----------------------------------------------------
	void ShowGui();
	virtual void DerivativeGui();

	//--------- accessor -----------------------------------------------------
	const std::list<std::shared_ptr<BaseBullet>>& GetBullets(BulletID id) const;
	std::string_view GetTypeName() const override{ return "BulletContainer"; }

protected:
	//===================================================================*/
	//					protected variables
	//===================================================================*/
	std::unordered_map<BulletID,
		std::list<std::shared_ptr<BaseBullet>>> typedBullets_;
	BaseBullet* editBullet_;
};
