#pragma once

//* engine
#include "Collider.h"

/* ========================================================================
/*		球体コライダー
/* ===================================================================== */
class SphereCollider :
	public Collider{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	SphereCollider() = default;
	SphereCollider(bool isEnuble = true);
	~SphereCollider()override = default;

	void Initialize(float radius);
	void Update(const CxMath::Vector3& position, const CxMath::Quaternion& rotate)override;
	void Draw()override;
	void ShowGui()override;

	//* collision ==========================================*//
	void OnCollisionEnter([[maybe_unused]] Collider* other)override{};
	void OnCollisionStay([[maybe_unused]] Collider* other)override{};
	void OnCollisionExit([[maybe_unused]] Collider* other)override{};

	float GetColliderRadius()const override { return shape_.radius; }
	void SetRadius(float radius) { shape_.radius = radius; }

protected:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	Sphere shape_; //衝突判定用のobb

public:
	//===================================================================*/
	//                   getter/setter
	//===================================================================*/
	const CxMath::Vector3& GetCenter()const override;

	const std::variant<Sphere, OBB>& GetCollisionShape() override;
};