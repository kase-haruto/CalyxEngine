#pragma once
/* ========================================================================
/* include
/* ===================================================================== */
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Objects/Transform/Transform.h>

class RailCamera 
	: public Camera3d {
public:
	RailCamera();
	RailCamera(const std::string& name);
	~RailCamera() = default;

	void Initialize();
	void Update(float dt) override;

	Vector3 GetPosition();
	const Vector3& GetRotation() const { return transform_.rotate; }
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }

	std::string_view GetTypeName() const override{ return "RailCamera"; }

private:
	float t_ = 0.0f;
	float speed_ = 20.0f;         // 速度
	float zTiltOffset_ = 0.0f;    // 現在の傾き
	float targetTilt_ = 0.0f;     // 目標の傾き
	float tiltAngle_ = 0.3f;      // 最大傾き（ラジアン）
	float tiltLerpSpeed_ = 10.0f; // 傾き補間速度
};
