#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/objects/Collider/SphereCollider.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>

/* ========================================================================
/* bullet 基底クラス
/* ===================================================================== */
class BaseBullet :
	public Actor{
public:
	//===================================================================*/
	//			public function
	//===================================================================*/
	BaseBullet() = default;
	BaseBullet(const std::string& modelName,const std::string& name);
	virtual ~BaseBullet()override = default;

	virtual void OnShot();
	virtual void ShootInitialize(const CxMath::Vector3& initPos, const CxMath::Vector3& velocity);
	void Initialize() override {}
	void Update(float dt)override;
	void DerivativeGui()override;

	//--------- collider -------------------------------------------------
	void OnCollisionEnter(Collider* other)override;
	void OnCollisionStay([[maybe_unused]] Collider* other)override {}
	void OnCollisionExit([[maybe_unused]] Collider* other)override {}

	//--------- accessor -------------------------------------------------
	Collider* GetCollider() { return BaseGameObject::GetCollider(); }

private:
	//===================================================================*/
	//private methods
	//===================================================================*/

protected:
	//===================================================================*/
	//			protected variables
	//===================================================================*/
	float lifeTime_ = 3.0f;      // 弾の寿命（秒）
	float currentTime_ = 0.0f;   // 経過時間
};

