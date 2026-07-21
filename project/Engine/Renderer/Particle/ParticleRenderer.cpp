#include "ParticleRenderer.h"

#include "Engine/Assets/Manager/AssetManager.h"
#include "Engine/Assets/Database/AssetDatabase.h"
#include "Engine/Graphics/Context/GraphicsGroup.h"

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
	const std::vector<std::shared_ptr<CalyxEngine::FxEmitter>>&    cpuEmitters,
	const std::vector<std::shared_ptr<CalyxEngine::GpuFxEmitter>>& gpuEmitters,
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
		for(auto& em : gpuEmitters) {
			if(!em || !em->IsDrawEnable()) continue;

			auto psoGpu = pipelineService->GetPipelineSet(
				PipelineTag::Object::GpuParticle,em->GetBlendMode());
			pipelineService->SetCommand(psoGpu,cmdList);

			if(auto* cam = CameraManager::GetActive())
				cam->SetCommand(cmdList,PipelineType::StructuredObject);

			em->GetMaterialBuffer().SetCommand(cmdList,1);
			cmdList->SetGraphicsRootDescriptorTable(3,em->GetTextureHandle());
			cmdList->SetGraphicsRootDescriptorTable(4,em->GetNoiseMaskTextureHandle());
			DrawGpuBillboards(cmdList,
							  em->GetDrawInstanceCount(),
							  em->GetParticleSrv());
		}
	}

	RenderTrails(cpuEmitters,pipelineService,cmdList);
}

uint32_t ParticleRenderer::NextPowerOfTwo(uint32_t value) {
	if(value<=1u) return 1u;
	--value; value|=value>>1u; value|=value>>2u; value|=value>>4u;
	value|=value>>8u; value|=value>>16u;
	return value+1u;
}

/////////////////////////////////////////////////////////////////////////////////////////
//    CPU Trail履歴からRibbon Meshを構築し、永続Map済みDynamic Bufferへ転送して描画する
/////////////////////////////////////////////////////////////////////////////////////////
void ParticleRenderer::RenderTrails(
	const std::vector<std::shared_ptr<CalyxEngine::FxEmitter>>& emitters,
	PipelineService* pipelineService,ID3D12GraphicsCommandList* cmdList) {
	auto* camera=CameraManager::GetActive();
	if(!camera) return;
	auto device=GraphicsGroup::GetInstance()->GetDevice();
	auto* textureManager=CalyxEngine::AssetManager::GetInstance()->GetTextureManager();
	std::unordered_map<const CalyxEngine::FxEmitter*,bool> active;

	for(const auto& emitter:emitters) {
		if(!emitter) continue;
		active[emitter.get()]=true;
		const auto& trail=emitter->GetTrailEmitter();
		if(!trail.Settings().enabled || !emitter->IsDrawEnable()) continue;
		const auto& settings=trail.Settings();
		MeshResource* geometryMesh=nullptr;
		if(settings.geometryMode!=CalyxEngine::TrailGeometryMode::Ribbon) {
			std::string modelPath=settings.geometryModelPath;
			if(settings.geometryModelGuid.isValid()) {
				if(auto* record=AssetDatabase::GetInstance()->Get(settings.geometryModelGuid);
					record && record->type==AssetType::Model) modelPath=record->sourcePath.filename().string();
			}
			if(!modelPath.empty()) {
				auto* modelManager=CalyxEngine::AssetManager::GetInstance()->GetModelManager();
				if(!modelManager->IsModelLoaded(modelPath)) modelManager->LoadModel(modelPath);
				else geometryMesh=&modelManager->GetMeshResource(modelPath);
			}
		}
		CalyxEngine::TrailMeshBuilder::Build(trail,camera->GetTranslate(),trailMeshScratch_,geometryMesh);
		if(trailMeshScratch_.vertices.empty() || trailMeshScratch_.indices.empty()) continue;

		auto& state=trailStates_[emitter.get()];
		if(!state.materialBuffer.GetResource()) state.materialBuffer.Initialize(device);
		const uint32_t requiredVertices=static_cast<uint32_t>(trailMeshScratch_.vertices.size());
		const uint32_t requiredIndices=static_cast<uint32_t>(trailMeshScratch_.indices.size());
		if(requiredVertices>state.vertexCapacity) {
			state.vertexCapacity=NextPowerOfTwo(requiredVertices);
			state.vertexBuffer.Initialize(device,state.vertexCapacity);
		}
		if(requiredIndices>state.indexCapacity) {
			state.indexCapacity=NextPowerOfTwo(requiredIndices);
			state.indexBuffer.Initialize(device,state.indexCapacity);
		}
		state.vertexBuffer.TransferVectorData(trailMeshScratch_.vertices);
		state.indexBuffer.TransferVectorData(trailMeshScratch_.indices);

		const auto& m=settings.material;
		const bool hasNoise=settings.noiseTextureGuid.isValid() || !settings.noiseTexturePath.empty();
		TrailGpuMaterial gpu{};
		gpu.color=m.color;
		gpu.baseTilingScroll={m.baseTiling.x,m.baseTiling.y,m.baseScrollSpeed.x,m.baseScrollSpeed.y};
		gpu.noiseTilingScroll={m.noiseTiling.x,m.noiseTiling.y,m.noiseScrollSpeed.x,m.noiseScrollSpeed.y};
		gpu.noiseDistortion={hasNoise?1.0f:0.0f,m.noiseStrength,m.distortionStrength,trail.GetTime()};
		gpu.dissolve={m.dissolveEnabled?1.0f:0.0f,m.dissolveStart,m.dissolveEnd,m.dissolveSoftness};
		gpu.dissolveEdge={m.dissolveEdgeWidth,m.dissolveEdgeEmissive,0,0};
		gpu.dissolveEdgeColor={m.dissolveEdgeColor.x,m.dissolveEdgeColor.y,m.dissolveEdgeColor.z,1};
		gpu.fadeClip={m.headFade,m.tailFade,m.alphaClipThreshold,0};
		gpu.emissiveColorIntensity={m.emissiveColor.x,m.emissiveColor.y,m.emissiveColor.z,m.emissiveIntensity};
		state.materialBuffer.TransferData(gpu);

		if(state.baseGuid!=settings.baseTextureGuid || state.basePath!=settings.baseTexturePath || state.baseTexture.ptr==0) {
			state.baseGuid=settings.baseTextureGuid;
			state.basePath=settings.baseTexturePath;
			state.baseTexture=settings.baseTextureGuid.isValid()?textureManager->LoadTexture(settings.baseTextureGuid):textureManager->LoadTexture(settings.baseTexturePath);
			if(!state.baseTexture.ptr) state.baseTexture=textureManager->LoadTexture("Textures/white1x1.dds");
		}
		if(state.noiseGuid!=settings.noiseTextureGuid || state.noisePath!=settings.noiseTexturePath || state.noiseTexture.ptr==0) {
			state.noiseGuid=settings.noiseTextureGuid;
			state.noisePath=settings.noiseTexturePath;
			state.noiseTexture=settings.noiseTextureGuid.isValid()?textureManager->LoadTexture(settings.noiseTextureGuid):textureManager->LoadTexture(settings.noiseTexturePath.empty()?"Textures/white1x1.dds":settings.noiseTexturePath);
			if(!state.noiseTexture.ptr) state.noiseTexture=textureManager->LoadTexture("Textures/white1x1.dds");
		}

		auto pipeline=pipelineService->GetPipelineSet(PipelineTag::Object::Trail,settings.blendMode);
		pipelineService->SetCommand(pipeline,cmdList);
		camera->SetRootCommand(cmdList,0);
		state.materialBuffer.SetCommand(cmdList,1);
		cmdList->SetGraphicsRootDescriptorTable(2,state.baseTexture);
		cmdList->SetGraphicsRootDescriptorTable(3,state.noiseTexture);
		state.vertexBuffer.SetCommand(cmdList);
		state.indexBuffer.SetCommand(cmdList);
		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		cmdList->DrawIndexedInstanced(requiredIndices,1,0,0,0);
	}

	std::erase_if(trailStates_,[&](const auto& pair){return !active.contains(pair.first);});
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//		GPUパーティクルのまとめ描きユーティリティ
///////////////////////////////////////////////////////////////////////////////////////////////////
void ParticleRenderer::RenderGrouped(const std::string&                                    modelPath,
									 const std::vector<CalyxEngine::ParticleConstantData>& gpuUnits,
									 ID3D12GraphicsCommandList*                            cmdList) {
	if(gpuUnits.empty()) return;

	ModelData& model = CalyxEngine::AssetManager::GetInstance()->GetModelManager()->GetModelData(modelPath);
	if(model.meshResource.Indices().empty()) return;

	auto device = GraphicsGroup::GetInstance()->GetDevice().Get();
	EnsureMeshIsReady(model.meshResource,device);

	// 一時バッファをローカルで作成
	DxStructuredBuffer<CalyxEngine::ParticleConstantData> tempBuffer;
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

void ParticleRenderer::DrawGpuBillboards(ID3D12GraphicsCommandList*  cmdList,
										 UINT                        instanceCount,
										 D3D12_GPU_DESCRIPTOR_HANDLE particleHandle) {
	if(instanceCount == 0) return;

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	cmdList->SetGraphicsRootDescriptorTable(2, particleHandle);
	cmdList->DrawInstanced(4, instanceCount, 0, 0);
}
