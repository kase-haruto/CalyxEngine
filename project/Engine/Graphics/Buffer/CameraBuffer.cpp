#include "CameraBuffer.h"



/* ========================================================================
/*		include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void Camera3DBuffer::Initialize(ID3D12Device* device){
	buffer_.Initialize(device);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void Camera3DBuffer::Update(const CxMath::Matrix4x4& view, const CxMath::Matrix4x4& proj, const CxMath::Vector3& worldPos){
	data_.view = view;
	data_.projection = proj;
	data_.viewProjection = CxMath::Matrix4x4::Multiply(view, proj);
	data_.worldPosition = worldPos;

	  // ビルボード用のカメラの右・上ベクトル（View行列のX列とY列）
	data_.camRight = CxMath::Vector3(view.m[0][0], view.m[1][0], view.m[2][0]);
	data_.camUp = CxMath::Vector3(view.m[0][1], view.m[1][1], view.m[2][1]);

	// バッファにデータを転送
	buffer_.TransferData(data_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		コマンドを積む
/////////////////////////////////////////////////////////////////////////////////////////
void Camera3DBuffer::SetCommand(ID3D12GraphicsCommandList* cmdList, PipelineType pipelineType) {
	uint32_t rootParameterIndex = 0;

	if (pipelineType == PipelineType::Object3D || pipelineType == PipelineType::SkinningObject3D){
		rootParameterIndex = 4;
	} else if (pipelineType == PipelineType::Line || pipelineType == PipelineType::Skybox){
		rootParameterIndex = 1;
	} 

	buffer_.SetCommand(cmdList, rootParameterIndex);
}
