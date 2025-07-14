#include "ParticleRenderer.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Assets/Model/Modelmanager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>


void ParticleRenderer::Render(
	const std::vector<std::shared_ptr<FxEmitter>>& emitters,
	PipelineService* pipelineService,
	ID3D12GraphicsCommandList* cmdList
){
	auto psoSet = pipelineService->GetPipelineSet(PipelineTag::Object::Particle, BlendMode::ADD);
	psoSet.SetCommand(cmdList);

	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();

	for (const auto& emitter : emitters){
		if (!emitter || !emitter->IsDrawEnable() || emitter->GetUnits().empty()) continue;

		// マテリアル・テクスチャ設定
		emitter->GetMaterialBuffer().SetCommand(cmdList, 1);
		auto textureHandle = TextureManager::GetInstance()->LoadTexture("Textures/" + emitter->GetTexturePath());
		cmdList->SetGraphicsRootDescriptorTable(3, textureHandle);



		// モデル取得＆初期化
		ModelData& model = ModelManager::GetInstance()->GetModelData(emitter->GetModelPath());
		if (model.meshData.indices.empty()) continue;

		EnsureModelIsReady(model, device);

		DrawModelInstanced(
			model, cmdList,
			static_cast< UINT >(emitter->GetUnits().size()),
			emitter->GetInstanceBuffer().GetGpuHandle()
		);
	}
}

void ParticleRenderer::RenderGrouped(const std::string& modelPath,
									 const std::vector<ParticleConstantData>& gpuUnits,
									 ID3D12GraphicsCommandList* cmdList){
	if (gpuUnits.empty()) return;

	ModelData& model = ModelManager::GetInstance()->GetModelData(modelPath);
	if (model.meshData.indices.empty()) return;

	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();
	EnsureModelIsReady(model, device);

	// 一時バッファをローカルで作成
	DxStructuredBuffer<ParticleConstantData> tempBuffer;
	tempBuffer.Initialize(device, static_cast< UINT >(gpuUnits.size()));
	tempBuffer.TransferVectorData(gpuUnits);
	tempBuffer.CreateSrv(device);

	DrawModelInstanced(model, cmdList,
					   static_cast< UINT >(gpuUnits.size()),
					   tempBuffer.GetGpuHandle());
}

void ParticleRenderer::EnsureModelIsReady(ModelData& model, ID3D12Device* device){
	if (!model.vertexBuffer.IsInitialized()){
		model.vertexBuffer.Initialize(device, static_cast< UINT >(model.meshData.vertices.size()));
		model.vertexBuffer.TransferVectorData(model.meshData.vertices);
	}
	if (!model.indexBuffer.IsInitialized()){
		model.indexBuffer.Initialize(device, static_cast< UINT >(model.meshData.indices.size()));
		model.indexBuffer.TransferVectorData(model.meshData.indices);
	}
}

void ParticleRenderer::DrawModelInstanced(ModelData& model,
										  ID3D12GraphicsCommandList* cmdList,
										  UINT instanceCount,
										  D3D12_GPU_DESCRIPTOR_HANDLE instanceHandle){
	model.vertexBuffer.SetCommand(cmdList);
	model.indexBuffer.SetCommand(cmdList);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->SetGraphicsRootDescriptorTable(2, instanceHandle);
	cmdList->DrawInstanced(4, instanceCount, 0, 0);
}