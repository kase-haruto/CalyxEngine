#pragma once
#include <Engine/Objects/3D/Actor/Actor.h>
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/objects/Collider/SphereCollider.h>
/* ========================================================================
/* enemy
/* ===================================================================== */
class Enemy :
    public Actor{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	Enemy() = default;
	Enemy(const std::string& modelName,const std::string objName);

	virtual ~Enemy() = default;

	void InitializeEffect();
	void Initialize()override;
	void Update()override;


	void OnCollisionEnter(Collider* other)override;
	void OnCollisionStay([[maybe_unused]]Collider* other)override {};
	void OnCollisionExit([[maybe_unused]] Collider* other)override {};
	const Vector3 GetCenterPos()const override;
	void SetPosition(const Vector3& position){
		worldTransform_.translation = position;
	};

	void SetParent(WorldTransform* parent);
	void SetParent(SceneObject* newParent)override;
private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	void Move();
	void Shoot();

private:
	//===================================================================*/
	//					private variables
	//===================================================================*/
	bool isHit_ = false;		// 衝突フラグ

	Vector3 basePosition_{};   // サイン波の基準位置
	float waveTime_ = 0.0f;     // 経過時間
	float waveAmplitude_ = 1.0f; // 振れ幅
	float waveSpeed_ = 2.0f;     // サイン波の速さ

};

