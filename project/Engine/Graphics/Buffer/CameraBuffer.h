#pragma once

//engine
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/PipelineType.h>

//math
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>

// 定数バッファ用データ構造体
struct Camera3DForGPU {
	CxMath::Matrix4x4 view;
	CxMath::Matrix4x4 projection;
	CxMath::Matrix4x4 viewProjection;
	CxMath::Vector3 worldPosition;
	float padding1;		// 16バイトアライメント調整

	CxMath::Vector3 camRight;	// ビルボード用のカメラ右方向
	float padding2;
	CxMath::Vector3 camUp;		// ビルボード用のカメラ上方向
	float padding3;
};

class Camera3DBuffer{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	void Initialize(ID3D12Device* device);
	void Update(const CxMath::Matrix4x4& view, const CxMath::Matrix4x4& proj, const CxMath::Vector3& worldPos);
	void SetCommand(ID3D12GraphicsCommandList* cmdList, PipelineType pipelineType);

private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	Camera3DForGPU data_;
	DxConstantBuffer<Camera3DForGPU> buffer_;
};

