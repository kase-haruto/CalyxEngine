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
	CalyxEngine::Matrix4x4 view;
	CalyxEngine::Matrix4x4 projection;
	CalyxEngine::Matrix4x4 viewProjection;
	CalyxEngine::Vector3	 worldPosition;
	float				 padding1; // 16バイトアライメント調整

	CalyxEngine::Vector3 camRight; // ビルボード用のカメラ右方向
	float			   padding2;
	CalyxEngine::Vector3 camUp; // ビルボード用のカメラ上方向
	float			   padding3;
	CalyxEngine::Vector3 camForward; // ビルボード用のカメラ前方向
	float			   padding4;

	CalyxEngine::Vector2 viewportSize; // ビューポートサイズ
	float			   padding5[2];  // 16B アライン
};

class Camera3DBuffer {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	void Initialize(ID3D12Device* device);
	void Update(const CalyxEngine::Matrix4x4& view, const CalyxEngine::Matrix4x4& proj, const CalyxEngine::Vector3& worldPos, const CalyxEngine::Vector2& viewportSize);
	void SetCommand(ID3D12GraphicsCommandList* cmdList, PipelineType pipelineType) const;
	void SetCommand(ID3D12GraphicsCommandList* cmdList, uint32_t rootIndex) const;
private:
	//===================================================================*/
	//                   private methods
	//===================================================================*/
	Camera3DForGPU					 data_;
	DxConstantBuffer<Camera3DForGPU> buffer_;
};
