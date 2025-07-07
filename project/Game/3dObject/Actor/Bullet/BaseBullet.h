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
	virtual ~BaseBullet();

	virtual void ShootInitialize(const Vector3 initPos, const Vector3 velocity);
	void Initialize() override {};
	void Update()override;
	void DerivativeGui()override;

	void OnCollisionEnter(Collider* other)override;
	void OnCollisionStay([[maybe_unused]] Collider* other)override {};
	void OnCollisionExit([[maybe_unused]] Collider* other)override {};

	//--------- accessor ---------------------------------------------------
private:
	//===================================================================*/
	//private methods
	//===================================================================*/
	void OnShot();

protected:
	//===================================================================*/
	//			protected variables
	//===================================================================*/
	float lifeTime_ = 3.0f;      // 弾の寿命（秒）
	float currentTime_ = 0.0f;   // 経過時間


};

