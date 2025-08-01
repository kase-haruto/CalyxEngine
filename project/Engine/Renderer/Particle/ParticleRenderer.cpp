#include "ParticleRenderer.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Assets/Model/Modelmanager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>


void ParticleRenderer::Render(
	const std::vector<std::shared_ptr<FxEmitter>>& cpuEmitters,
	const std::vector<std::shared_ptr<GpuFxEmitter>>& gpuEmitters,
	PipelineService* pipelineService,
	ID3D12GraphicsCommandList* cmdList){
	if (cpuEmitters.empty() && gpuEmitters.empty()) return;

	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

	// ───────────── CPU パーティクル ─────────────
	if (!cpuEmitters.empty()){
		auto psoCpu = pipelineService->GetPipelineSet(
			PipelineTag::Object::Particle, BlendMode::SUB);
		pipelineService->SetCommand(psoCpu, cmdList);          // ① 先に PSO

		if (auto* cam = CameraManager::GetActive())            // ② その後 CBV
			cam->SetCommand(cmdList, PipelineType::StructuredObject);

		for (auto& em : cpuEmitters){
			if (!em || !em->IsDrawEnable() || em->GetUnits().empty()) continue;

			em->GetMaterialBuffer().SetCommand(cmdList, 1);
			auto tex = TextureManager::GetInstance()->LoadTexture("Textures/" + em->GetTexturePath());
			cmdList->SetGraphicsRootDescriptorTable(3, tex);

			ModelData& model =
				ModelManager::GetInstance()->GetModelData(em->GetModelPath());
			if (model.meshData.indices.empty()) continue;
			EnsureModelIsReady(model, device);

			DrawModelInstanced(model, cmdList,
							   static_cast< UINT >(em->GetUnits().size()),
							   em->GetInstanceBuffer().GetGpuSrvHandle());
		}
	}

	// ───────────── GPU パーティクル ─────────────
	if (!gpuEmitters.empty()){
		auto psoGpu = pipelineService->GetPipelineSet(
			PipelineTag::Object::GpuParticle, BlendMode::ADD);
		pipelineService->SetCommand(psoGpu, cmdList);

		if (auto* cam = CameraManager::GetActive())
			cam->SetCommand(cmdList, PipelineType::StructuredObject);

		for (auto& em : gpuEmitters){
			if (!em) continue;

			em->GetMaterialBuffer().SetCommand(cmdList, 1);
			auto tex = TextureManager::GetInstance()->LoadTexture("Textures/" + em->GetTexturePath());
			cmdList->SetGraphicsRootDescriptorTable(3, tex);

			ModelData& model =
				ModelManager::GetInstance()->GetModelData(em->GetModelPath());
			if (model.meshData.indices.empty()) continue;
			EnsureModelIsReady(model, device);

			DrawModelInstanced(model, cmdList,
							   GpuFxEmitter::kMaxParticles,
							   em->GetParticleSrv());
		}
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
					   tempBuffer.GetGpuSrvHandle());
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