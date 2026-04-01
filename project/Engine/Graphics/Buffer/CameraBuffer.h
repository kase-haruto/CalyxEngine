#pragma once

// engine
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Pipeline/PipelineType.h>

// math
#include "Engine/Foundation/Math/Vector2.h"

#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>

// 定数バッファ用データ構造体
struct Camera3DForGPU {
	CalyxMath::Matrix4x4 view;
	CalyxMath::Matrix4x4 projection;
	CalyxMath::Matrix4x4 viewProjection;
	CalyxMath::Vector3	 worldPosition;
	float				 padding1; // 16バイトアライメント調整

	CalyxMath::Vector3 camRight; // ビルボード用のカメラ右方向
	float			   padding2;
	CalyxMath::Vector3 camUp; // ビルボード用のカメラ上方向
	float			   padding3;
	CalyxMath::Vector3 camForward; // ビルボード用のカメラ前方向
	float			   padding4;

	CalyxMath::Vector2 viewportSize; // ビューポートサイズ
	float			   padding5[2];  // 16B アライン
};

class Camera3DBuffer {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	void Initialize(ID3D12Device* device);
	void Update(const CalyxMath::Matrix4x4& view, const CalyxMath::Matrix4x4& proj, const CalyxMath::Vector3& worldPos, const CalyxMath::Vector2& viewportSize);
	void SetCommand(ID3D12GraphicsCommandList* cmdList, PipelineType pipelineType);
	void SetCommand(ID3D12GraphicsCommandList* cmdList, uint32_t rootIndex);
private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	Camera3DForGPU					 data_;
	DxConstantBuffer<Camera3DForGPU> buffer_;
};