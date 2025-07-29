#pragma once

#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>

struct Vector3;
struct Vector2;

class FollowCamera:
public BaseCamera{
public:
	//===================================================================*/
	//							public Methods
	//===================================================================*/

	FollowCamera();
	~FollowCamera() = default;

	void Update(float dt)override;
	void AlwaysUpdate(float dt)override;
	void ShowGui()override;

	std::string_view GetTypeName() const override{ return "FollowCamera"; }
private:
	//===================================================================*/
	//							private Methods
	//===================================================================*/	
	Vector3 CalculateOffset();			//* オフセットの計算
	void Turning(float dt);				//* 旋回
	void Adulation();					//* 追従
	void UpdateMatrix()override;		//* 行列の更新


public:
	//===================================================================*/
	//					getter/setter
	//===================================================================*/
#pragma region アクセッサ
	//* 追従対象のセット
	void SetTarget(const EulerTransform* target){ target_ = target; }

	void SetViewpoint(const Vector3& viewpoint){ viewpoint_ = viewpoint; }

	void SetRotate(const Vector3& rotate){ transform_.rotate = rotate; }

#pragma endregion

private:
	//===================================================================*/
	//							private Methods
	//===================================================================*/

	const EulerTransform* target_ = nullptr;	//* 追従対象
	Vector3 viewpoint_;							//* 注視点
	Vector3 interTarget_;						//* 追従対象の残像座標
	Vector3 offset = {0.0f,3.0f,-14.0f};		//* 追従対象からのオフセット
	float rotateSpeed = 2.0f;					//* 回転の速度

	Vector2 destinationAngle_ {0.0f,0.0f};		//* 目標角度

	Matrix4x4 rotateMatrix_;					//* 回転行列

};

