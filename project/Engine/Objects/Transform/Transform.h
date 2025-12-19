#pragma once


#include <Engine/Graphics/Buffer/DxConstantBuffer.h>

#include <Data/Engine/Configs/Scene/Objects/Transform/UvTransformConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Transform/WorldTransformConfig.h>

// math
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Matrix4x4.h>

// c++
#include <string>

enum class RotationSource {
	Euler,
	Quaternion
};

struct TransformationMatrix{
	CxMath::Matrix4x4 world;
	CxMath::Matrix4x4 WorldInverseTranspose;
};

struct EulerTransform{
	CxMath::Vector3 scale;
	CxMath::Vector3 rotate;
	CxMath::Vector3 translate;

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
	CxMath::Vector3 scale;
	CxMath::Quaternion rotate;
	CxMath::Vector3 translate;
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
	virtual void Update([[maybe_unused]]const CxMath::Matrix4x4& viewProMatrix){}
	virtual void Update(){}
	virtual void SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
							UINT rootParameterIndex)const{
		DxBuffer::SetCommand(commandList, rootParameterIndex);
	};

	//--------- ImGui ---------------------------------------------------
	virtual void ShowImGui(const std::string& lavel = "Transform");

	//--------- accessor -------------------------------------------------
	virtual CxMath::Vector3 GetWorldPosition()const;

public:
	//========================================================================*/
	//	public variables
	//========================================================================*/
	CxMath::Vector3 scale;
	CxMath::Quaternion rotation;
	CxMath::Vector3 translation;

	CxMath::Vector3 eulerRotation;

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

	virtual void Update(const CxMath::Matrix4x4& viewProMatrix) override;
	void Update()override;

	CxMath::Vector3 GetForward()const;
	//--- コンフィグ同期 ---
	void ApplyConfig(const WorldTransformConfig& config);
	WorldTransformConfig ExtractConfig();
public:
	bool inheritScale = true;//< 親のスケールを継承するか
};

//============================================================================*/
//	json serialization
//============================================================================*/
