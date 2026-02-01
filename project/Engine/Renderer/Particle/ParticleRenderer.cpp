#include "ParticleRenderer.h"

#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Assets/Model/Modelmanager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Mesh/MeshData.h>

///////////////////////////////////////////////////////////////////////////////////////////////
//		パーティクル描画
///////////////////////////////////////////////////////////////////////////////////////////////
void ParticleRenderer::Render(
	const std::vector<std::shared_ptr<CalyxEffect::FxEmitter>>&    cpuEmitters,
	const std::vector<std::shared_ptr<CalyxEffect::GpuFxEmitter>>& gpuEmitters,
	PipelineService*                                               pipelineService,
	ID3D12GraphicsCommandList*                                     cmdList) {
	if(cpuEmitters.empty() && gpuEmitters.empty()) return;

	//ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

	// ───────────── CPU パーティクル ─────────────
	if(!cpuEmitters.empty()) {
		for(auto& em : cpuEmitters) {
			auto psoCpu = pipelineService->GetPipelineSet(
				PipelineTag::Object::Particle,em->GetBlendMode());
			pipelineService->SetCommand(psoCpu,cmdList);

			if(auto* cam = CameraManager::GetActive())
				cam->SetCommand(cmdList,PipelineType::StructuredObject);

			if(!em || !em->IsDrawEnable() || em->GetUnits().empty()) continue;

			em->SetCommand(cmdList);

			MeshResource& mesh = em->GetMeshResource();
			if(mesh.Indices().empty()) continue;
			//EnsureMeshIsReady(mesh,device);

			DrawMeshInstanced(mesh,cmdList,
							  static_cast<UINT>(em->GetUnits().size()),
							  em->GetInstanceBuffer().GetGpuSrvHandle());
		}
	}

	// ───────────── GPU パーティクル ─────────────
	if(!gpuEmitters.empty()) {
		auto psoGpu = pipelineService->GetPipelineSet(
			PipelineTag::Object::GpuParticle,BlendMode::ADD);
		pipelineService->SetCommand(psoGpu,cmdList);

		if(auto* cam = CameraManager::GetActive())
			cam->SetCommand(cmdList,PipelineType::StructuredObject);

		for(auto& em : gpuEmitters) {
			if(!em) continue;

			em->GetMaterialBuffer().SetCommand(cmdList,1);
			auto tex = TextureManager::GetInstance()->LoadTexture("Textures/" + em->GetTexturePath());
			cmdList->SetGraphicsRootDescriptorTable(3,tex);
			MeshResource& mesh = em->GetMeshResource();

			if(mesh.Indices().empty()) continue;
			//EnsureMeshIsReady(mesh,device);

			DrawMeshInstanced(mesh,cmdList,
							  CalyxEffect::GpuFxEmitter::kMaxParticles,
							  em->GetParticleSrv());
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//		GPUパーティクルのまとめ描きユーティリティ
///////////////////////////////////////////////////////////////////////////////////////////////////
void ParticleRenderer::RenderGrouped(const std::string&                                    modelPath,
									 const std::vector<CalyxEffect::ParticleConstantData>& gpuUnits,
									 ID3D12GraphicsCommandList*                            cmdList) {
	if(gpuUnits.empty()) return;

	ModelData& model = ModelManager::GetInstance()->GetModelData(modelPath);
	if(model.meshResource.Indices().empty()) return;

	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();
	EnsureMeshIsReady(model.meshResource,device);

	// 一時バッファをローカルで作成
	DxStructuredBuffer<CalyxEffect::ParticleConstantData> tempBuffer;
	tempBuffer.Initialize(device,static_cast<UINT>(gpuUnits.size()));
	tempBuffer.TransferVectorData(gpuUnits);
	tempBuffer.CreateSrv(device);

	DrawMeshInstanced(model.meshResource,cmdList,
					  static_cast<UINT>(gpuUnits.size()),
					  tempBuffer.GetGpuSrvHandle());
}

///////////////////////////////////////////////////////////////////////////////////////////
//		メッシュ描画ユーティリティ
///////////////////////////////////////////////////////////////////////////////////////////
void ParticleRenderer::EnsureMeshIsReady(MeshResource& mesh,ID3D12Device* device) {
	if(!mesh.VertexBuffer().IsInitialized()) {
		mesh.VertexBuffer().Initialize(device,static_cast<UINT>(mesh.Vertices().size()));
		mesh.VertexBuffer().TransferVectorData(mesh.Vertices());
	}
	if(!mesh.IndexBuffer().IsInitialized()) {
		mesh.IndexBuffer().Initialize(device,static_cast<UINT>(mesh.Indices().size()));
		mesh.IndexBuffer().TransferVectorData(mesh.Indices());
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		メッシュインスタンス描画ユーティリティ
///////////////////////////////////////////////////////////////////////////////////////////////
void ParticleRenderer::DrawMeshInstanced(MeshResource&               mesh,
										 ID3D12GraphicsCommandList*  cmdList,
										 UINT                        instanceCount,
										 D3D12_GPU_DESCRIPTOR_HANDLE instanceHandle) {
	mesh.VertexBuffer().SetCommand(cmdList);
	mesh.IndexBuffer().SetCommand(cmdList);
	cmdList->IASetPrimitiveTopology(mesh.topology);
	cmdList->SetGraphicsRootDescriptorTable(2,instanceHandle);

	// インデックス描画に変更（インデックス数で描画）
	const UINT indexCount = static_cast<UINT>(mesh.Indices().size());
	cmdList->DrawIndexedInstanced(indexCount, instanceCount, 0, 0, 0);
}