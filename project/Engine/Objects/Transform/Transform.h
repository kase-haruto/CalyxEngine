#pragma once
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Quaternion.h>

#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>

// c++
#include <string>

enum class RotationSource {
	Euler,
	Quaternion
};

struct TransformationMatrix{
	Matrix4x4 world;
	Matrix4x4 WorldInverseTranspose;
};

struct EulerTransform{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;

	void Initialize(){
		scale = {1.0f,1.0f,1.0f};
		rotate = {0.0f,0.0f,0.0f};
		translate = {0.0f,0.0f,0.0f};
	}

	void ShowImGui(const std::string& lavel = "Transform");
};


struct Transform2D{
	Vector2 scale;
	float rotate;
	Vector2 translate;
	
	void Initialize(){
		scale = {1.0f,1.0f};
		rotate =0.0f;
		translate = {0.0f,0.0f};
	}
	void ShowImGui(const std::string& lavel = "Transform");
	Transform2DConfig ExtractConfig()const;
	void ShowImGui(Transform2DConfig& config, const std::string& lavel = "Transform");
	void ApplyConfig(const Transform2DConfig& config);
};

struct QuaternionTransform{
	Vector3 scale;
	Quaternion rotate;
	Vector3 translate;
};

//============================================================================*/
//	BaseTransform class
//============================================================================*/
class BaseTransform :
	public DxConstantBuffer<TransformationMatrix>{
public:
	//========================================================================*/
	//	public functions
	//========================================================================*/
	BaseTransform() = default;
	virtual ~BaseTransform() = default;

	//--------- main -----------------------------------------------------
	virtual void Initialize();
	virtual void Update([[maybe_unused]]const Matrix4x4& viewProMatrix){}
	virtual void Update(){}
	virtual void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
							UINT rootParameterIndex)const{
		DxBuffer::SetCommand(commandList, rootParameterIndex);
	};

	//--------- ImGui ---------------------------------------------------
	virtual void ShowImGui(const std::string& lavel = "Transform");

	//--------- accessor -------------------------------------------------
	virtual Vector3 GetWorldPosition()const;

public:
	//========================================================================*/
	//	public variables
	//========================================================================*/
	Vector3 scale;
	Quaternion rotation;
	Vector3 translation;

	Vector3 eulerRotation;

	TransformationMatrix matrix;
	BaseTransform* parent = nullptr;

	RotationSource rotationSource = RotationSource::Quaternion;

};

//============================================================================*/
//	worldTransform class
//============================================================================*/
class WorldTransform :
	public BaseTransform{
public:
	//========================================================================*/
	//	public functions
	//========================================================================*/
	WorldTransform() = default;
	~WorldTransform()override = default;

	virtual void Update(const Matrix4x4& viewProMatrix) override;
	void Update()override;

	Vector3 GetForward()const;
	//--- コンフィグ同期 ---
	void ApplyConfig(const WorldTransformConfig& config);
	WorldTransformConfig ExtractConfig();
public:
	
};

//============================================================================*/
//	json serialization
//============================================================================*/
