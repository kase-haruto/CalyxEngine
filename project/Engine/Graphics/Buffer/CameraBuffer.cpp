#include "CameraBuffer.h"

/* ========================================================================
/*		include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化処理
/////////////////////////////////////////////////////////////////////////////////////////
void Camera3DBuffer::Initialize(ID3D12Device* device) {
	buffer_.Initialize(device);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新処理
/////////////////////////////////////////////////////////////////////////////////////////
void Camera3DBuffer::Update(const CalyxEngine::Matrix4x4& view, const CalyxEngine::Matrix4x4& proj, const CalyxEngine::Vector3& worldPos, const CalyxEngine::Vector2& viewportSize) {
	data_.view			 = view;
	data_.projection	 = proj;
	data_.viewProjection = CalyxEngine::Matrix4x4::Multiply(view, proj);
	data_.worldPosition	 = worldPos;
	data_.viewportSize   = viewportSize;

	// ビルボード用のカメラの右・上基底ベクトル（View行列の行成分。RH想定なら転置された回転成分）
	data_.camRight	 = CalyxEngine::Vector3(view.m[0][0], view.m[1][0], view.m[2][0]);
	data_.camUp		 = CalyxEngine::Vector3(view.m[0][1], view.m[1][1], view.m[2][1]);
	data_.camForward = CalyxEngine::Vector3(view.m[0][2], view.m[1][2], view.m[2][2]);

	// バッファにデータを転送
	buffer_.TransferData(data_);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		コマンドを積む
/////////////////////////////////////////////////////////////////////////////////////////
void Camera3DBuffer::SetCommand(ID3D12GraphicsCommandList* cmdList, PipelineType pipelineType) const {
	uint32_t rootParameterIndex = 0;

	if(pipelineType == PipelineType::Object3D || pipelineType == PipelineType::SkinningObject3D) {
		rootParameterIndex = 4;
	} else if(pipelineType == PipelineType::Line || pipelineType == PipelineType::Skybox) {
		rootParameterIndex = 1;
	}

	buffer_.SetCommand(cmdList, rootParameterIndex);
}

void Camera3DBuffer::SetCommand(ID3D12GraphicsCommandList* cmdList, uint32_t rootIndex) const {
	buffer_.SetCommand(cmdList, rootIndex);
}
