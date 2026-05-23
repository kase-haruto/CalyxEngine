#include "ModelRenderer.h"
#include <Engine/Foundation/Debug/CxAssert.h>

/* ========================================================================
/* include space
/* ===================================================================== */
#include "Engine/Foundation/Math/Matrix3x4.h"

#include "Engine/Graphics/Context/GraphicsGroup.h"
#include "Engine/Graphics/Shadow/ShadowMap/ShadowMapSystem.h"
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <cstring>

ModelRenderer::ModelRenderer() {
	Microsoft::WRL::ComPtr<ID3D12Device5> device5;
	GraphicsGroup::GetInstance()->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5));

	if(device5) {
		raytracingSystem_ = std::make_unique<CalyxEngine::RaytracingSystem>();
		raytracingSystem_->Initialize(device5.Get());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル登録（ビルボードモード付き）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterStatic(BaseModel* model, const WorldTransform& transform, BillboardMode billMode, SceneObject* owner) {
	InstanceStatic inst{};
	inst.tf		 = transform;
	inst.dirty	 = true;
	inst.visible = false;
	inst.mode	 = billMode;
	inst.owner	 = owner;
	staticModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーションモデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterSkinned(CalyxEngine::AnimationModel* model, const WorldTransform& transform, SceneObject* owner) {
	InstanceSkinned inst{};
	inst.tf		 = transform;
	inst.dirty	 = true;
	inst.visible = false;
	inst.owner	 = owner;
	skinnedModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		クリア
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::Clear() {
	staticModels_.clear();
	skinnedModels_.clear();
	staticBatches_.clear();
	skinnedBatches_.clear();
	tempVisibleStatic_.clear();
	tempVisibleSkinned_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		フレーム開始
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BeginFrame() {
	for(auto& insts : staticModels_ | std::views::values) {
		for(auto& inst : insts) {
			inst.visible = false;
		}
	}
	for(auto& insts : skinnedModels_ | std::views::values) {
		for(auto& inst : insts) {
			inst.visible = false;
		}
	}
	staticBatches_.clear();
	skinnedBatches_.clear();

	// 毎フレ登録方式なのでクリア
	staticModels_.clear();
	skinnedModels_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		Dirty マーク
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::MarkStaticDirty(BaseModel* model, size_t index) {
	auto it = staticModels_.find(model);
	if(it == staticModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

void ModelRenderer::MarkSkinnedDirty(CalyxEngine::AnimationModel* model, size_t index) {
	auto it = skinnedModels_.find(model);
	if(it == skinnedModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台判定 & バッチ化
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::PreCullAndBatch(const Camera3d* camera, bool enableFrustumCulling) {

	// =========================================================
	// 静的モデル
	// =========================================================
	for(auto& [model, insts] : staticModels_) {
		if(!model) continue;
		if(!model->GetModelData()) continue;
		if(!model->GetIsDrawEnable()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;

		for(auto& inst : insts) {

			// World AABB 更新
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}

			// -------------------------
			// MainPass：カメラカリング
			// -------------------------
			inst.visible = !enableFrustumCulling || !camera || camera->IsVisible(inst.worldAABB);
		}
	}

	// =========================================================
	// スキンモデル（Static と完全に同じ扱い）
	// =========================================================
	for(auto& [model, insts] : skinnedModels_) {
		if(!model) continue;
		if(!model->GetModelData()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;

		for(auto& inst : insts) {

			// World AABB 更新
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}

			// -------------------------
			// MainPass：カメラカリング
			// -------------------------
			inst.visible = !enableFrustumCulling || !camera || camera->IsVisible(inst.worldAABB);
		}
	}

	CollectShadowCasters();

	// =========================================================
	// MainPass 用バッチ生成
	// =========================================================
	BuildStaticBatches();
	BuildSkinnedBatches();
}

void ModelRenderer::BuildAllVisibleBatches() {
	for(auto& [model, insts] : staticModels_) {
		if(!model || !model->GetModelData() || !model->GetIsDrawEnable()) continue;
		for(auto& inst : insts) {
			inst.visible = true;
			if(inst.dirty) {
				inst.worldAABB = model->GetModelData()->localAABB.Transform(inst.tf.matrix.world);
				inst.dirty = false;
			}
		}
	}

	for(auto& [model, insts] : skinnedModels_) {
		if(!model || !model->GetModelData()) continue;
		for(auto& inst : insts) {
			inst.visible = true;
			if(inst.dirty) {
				inst.worldAABB = model->GetModelData()->localAABB.Transform(inst.tf.matrix.world);
				inst.dirty = false;
			}
		}
	}

	CollectShadowCasters();

	BuildStaticBatches();
	BuildSkinnedBatches();
}

void ModelRenderer::CollectShadowCasters() {
	staticVisibleForShadow_.clear();
	skinnedVisibleForShadow_.clear();
	raytracingScene_.Clear();
	hasSceneBounds_ = false;

	auto expandSceneBounds = [&](const AABB& aabb) {
		if(!hasSceneBounds_) {
			sceneBounds_	= aabb;
			hasSceneBounds_ = true;
			return;
		}
		sceneBounds_.min_ = CalyxEngine::Vector3::Min(sceneBounds_.min_, aabb.min_);
		sceneBounds_.max_ = CalyxEngine::Vector3::Max(sceneBounds_.max_, aabb.max_);
	};

	for(auto& [model, insts] : staticModels_) {
		if(!model || !model->GetModelData() || !model->GetIsDrawEnable()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;
		for(auto& inst : insts) {
			if(inst.owner && !inst.owner->IsCastShadow()) continue;
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}
			staticVisibleForShadow_[model].push_back(inst.tf);
			expandSceneBounds(inst.worldAABB);
		}
	}

	for(auto& [model, insts] : skinnedModels_) {
		if(!model || !model->GetModelData() || !model->GetIsDrawEnable()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;
		for(auto& inst : insts) {
			if(inst.owner && !inst.owner->IsCastShadow()) continue;
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}
			skinnedVisibleForShadow_[model].push_back(inst.tf);
			expandSceneBounds(inst.worldAABB);
		}
	}
}

void ModelRenderer::BindRaytracingScene(ID3D12GraphicsCommandList* cmdList) const {
	if(!raytracingSystem_) return;
	ID3D12Resource* tlas = raytracingSystem_->GetTLAS();
	if(!tlas) return;

	cmdList->SetGraphicsRootShaderResourceView(
		10, // Space0, t3
		tlas->GetGPUVirtualAddress());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル・バッチ作成（BillboardParams も可視分だけ詰める）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildStaticBatches() {
	for(auto& [model, insts] : staticModels_) {
		if(!model->GetModelData() || !model->GetIsDrawEnable()) continue;

		std::vector<WorldTransform>		visTf;
		std::vector<GpuBillboardParams> visBb;
		visTf.reserve(insts.size());
		visBb.reserve(insts.size());

		for(auto& inst : insts) {
			if(!inst.visible) continue;
			visTf.push_back(inst.tf);

			GpuBillboardParams p{};
			p.mode = static_cast<uint32_t>(inst.mode);
			visBb.push_back(p);
		}
		if(visTf.empty()) continue;

		PipelineKey key{PipelineTag::Object::Object3d, model->GetBlendMode()};
		auto&		batch = staticBatches_[key];

		if(auto* item = FindCompatibleStaticBatch(batch, model)) {
			item->transforms.insert(item->transforms.end(), visTf.begin(), visTf.end());
			item->billboards.insert(item->billboards.end(), visBb.begin(), visBb.end());
			continue;
		}

		StaticBatchItem item;
		item.model = model;
		item.transforms.swap(visTf);
		item.billboards.swap(visBb);
		batch.emplace_back(std::move(item));
	}
}

ModelRenderer::StaticBatchItem* ModelRenderer::FindCompatibleStaticBatch(StaticBatch& batch, BaseModel* model) {
	if(!model || !model->GetModelData()) return nullptr;

	for(auto& item : batch) {
		BaseModel* base = item.model;
		if(!base || !base->GetModelData()) continue;
		if(base->GetModelData() != model->GetModelData()) continue;
		if(base->GetTexSrv().ptr != model->GetTexSrv().ptr) continue;
		if(base->GetEnvMapSrv().ptr != model->GetEnvMapSrv().ptr) continue;
		if(base->UsesRuntimeMaterialGraph() != model->UsesRuntimeMaterialGraph()) continue;
		if(base->UsesRuntimeMaterialGraph() && base->GetMaterialGuid() != model->GetMaterialGuid()) continue;
		if(std::memcmp(&base->GetMaterialForBatch(), &model->GetMaterialForBatch(), sizeof(Material)) != 0) continue;

		return &item;
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		スキンモデル・バッチ作成
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildSkinnedBatches() {
	for(auto& [model, insts] : skinnedModels_) {
		if(!model->GetModelData()) continue;

		tempVisibleSkinned_.clear();
		tempVisibleSkinned_.reserve(insts.size());

		for(auto& inst : insts) {
			if(inst.visible) {
				tempVisibleSkinned_.push_back(inst.tf);
			}
		}
		if(tempVisibleSkinned_.empty()) continue;

		PipelineKey key{PipelineTag::Object::SkinningObject3D, model->GetBlendMode()};
		auto&		batch = skinnedBatches_[key];
		batch.emplace_back(model, std::vector<WorldTransform>());
		batch.back().second.swap(tempVisibleSkinned_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		一斉描画
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::DrawAll(ID3D12GraphicsCommandList*		cmdList,
							ID3D12Device*					device,
							IRenderTarget*					rt,
							PipelineService*				psoService,
							LightLibrary*					lightLibrary,
							CalyxEngine::ShadowMapSystem* shadowMapSystem) {
	(void)rt;

	// Skinned meshes are converted to skinned vertex buffers once per frame.
	{
		bool computeSet = false;
		for(auto& [model, insts] : skinnedModels_) {
			if(!model || !model->GetModelData() || insts.empty()) continue;
			if(!computeSet) {
				const auto& ps = psoService->GetComputePipelineSet(PipelineTag::Compute::SkinningCompute);
				ps.SetCompute(cmdList);
				computeSet = true;
			}
			model->DispatchSkinning(psoService, cmdList);
		}
	}

	// Raytracing TLAS Build
	if(raytracingSystem_) {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> cmd4;
		Microsoft::WRL::ComPtr<ID3D12Device5>			   device5;

		if(SUCCEEDED(cmdList->QueryInterface(IID_PPV_ARGS(&cmd4))) &&
		   SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device5)))) {

			raytracingScene_.Clear();

			// Static Models
			for(auto& [model, transforms] : staticVisibleForShadow_) {
				if(!model->GetModelData()) continue;

				model->EnsureRaytracingBLAS(device5.Get(), cmd4.Get());

				if(model->HasBLAS()) {
					for(const auto& tf : transforms) {
						raytracingScene_.AddInstance(
							CalyxEngine::Matrix3x4::ToMatrix3x4(tf.matrix.world),
							model->GetBLAS(),
							0);
					}
				}
			}

			// Skinned Models
			for(auto& [model, transforms] : skinnedVisibleForShadow_) {
				if(!model->GetModelData()) continue;
				// TODO: Skinning update for BLAS if needed (Refitting/Rebuild)
				model->EnsureRaytracingBLAS(device5.Get(), cmd4.Get());

				if(model->HasBLAS()) {
					for(const auto& tf : transforms) {
						raytracingScene_.AddInstance(
							CalyxEngine::Matrix3x4::ToMatrix3x4(tf.matrix.world),
							model->GetBLAS(),
							0);
					}
				}
			}

			raytracingScene_.EnsureBuffer(device);
			raytracingScene_.Upload();

			D3D12_RESOURCE_BARRIER uav = {};
			uav.Type				   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uav.UAV.pResource		   = nullptr; // Global UAV barrier
			cmdList->ResourceBarrier(1, &uav);

			raytracingSystem_->BuildTLAS(cmd4.Get(), raytracingScene_);
		}
	}

	psoService->ResetState();

	// ------------------------------------------------------------
	// 静的モデル描画
	// ------------------------------------------------------------
	{
		PipelineKey lastKey{};
		bool		hasLast = false;
		bool		usingGeneratedPipeline = false;
		auto bindObject3DPassResources = [&]() -> bool {
			if(shadowMapSystem) {
				shadowMapSystem->BindForMainPass(cmdList);
			}

			BindRaytracingScene(cmdList);

			if(auto* cam = CameraManager::GetActive()) {
				cam->SetCommand(cmdList, PipelineType::Object3D);
			} else {
				return false;
			}

			lightLibrary->SetCommand(cmdList, PipelineType::Object3D);
			return true;
		};

		for(auto& [key, batch] : staticBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				if(!bindObject3DPassResources()) continue;

				lastKey = key;
				hasLast = true;
				usingGeneratedPipeline = false;
			}

			for(auto& item : batch) {
				BaseModel* model   = item.model;
				auto&	   visible = item.transforms;
				if(!model || visible.empty()) continue;

				if(model->UsesRuntimeMaterialGraph()) {
					if(auto material = model->GetMaterialAsset()) {
						CalyxEngine::MaterialGraphRuntimeShader shader = runtimeMaterialShaderCache_.GetOrCompileObject3DPixelShader(*material);
						if(shader.pixelShader) {
							const auto generatedSet = psoService->GetGeneratedMaterialObjectPipelineSet(model->GetBlendMode(), shader.pixelShader, shader.hash);
							psoService->SetCommand(generatedSet, cmdList);
							if(!bindObject3DPassResources()) continue;
							usingGeneratedPipeline = true;
						} else if(usingGeneratedPipeline) {
							const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
							psoService->SetCommand(ps, cmdList);
							if(!bindObject3DPassResources()) continue;
							usingGeneratedPipeline = false;
						}
					}
				} else if(usingGeneratedPipeline) {
					const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
					psoService->SetCommand(ps, cmdList);
					if(!bindObject3DPassResources()) continue;
					usingGeneratedPipeline = false;
				}

				const UINT need = static_cast<UINT>(item.billboards.size());
				if(need == 0) continue;
				CX_CHECK(item.transforms.size() == item.billboards.size(), "Assertion failed");

				model->EnsureBillboardCapacity(device, need);
				model->UploadBillboardParams(item.billboards);
				cmdList->SetGraphicsRootDescriptorTable(7, model->GetBillboardSrv());

				model->EnsureInstanceCapacity(device, need);
				model->UploadInstanceMatrices(visible);
				cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());

				if(model->UsesRuntimeMaterialGraph()) {
					model->TransferMaterial();
				}
				model->BindMaterialCB(cmdList);
				cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());
				cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(0));
				cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());

				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				model->BindVertexIndexBuffers(cmdList);

				const auto& meshResource = model->GetModelData()->meshResource;
				const auto& subMeshes = meshResource.SubMeshes();
				if(subMeshes.empty()) {
					const UINT indexCount = static_cast<UINT>(meshResource.Indices().size());
					cmdList->DrawIndexedInstanced(indexCount, need, 0, 0, 0);
				} else {
					for(const auto& subMesh : subMeshes) {
						cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv(subMesh.materialIndex));
						cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(subMesh.materialIndex));
						cmdList->DrawIndexedInstanced(subMesh.indexCount, need, subMesh.indexStart, 0, 0);
					}
				}
			}
		}
	}

	//------------------------------------------------------------
	// スキンメッシュ
	//------------------------------------------------------------
	{
		PipelineKey lastKey{};
		bool		hasLast = false;
		bool		usingGeneratedPipeline = false;

		for(auto& [key, batch] : skinnedBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				if(shadowMapSystem) {
					shadowMapSystem->BindForMainPass(cmdList);
				}

				BindRaytracingScene(cmdList);

				if(auto* cam = CameraManager::GetActive()) {
					cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
				} else {
					// 判定漏れ防止
					continue;
				}

				lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);

				lastKey = key;
				hasLast = true;
				usingGeneratedPipeline = false;
			}

			for(auto& [model, visible] : batch) {
				if(!model || visible.empty()) continue;

				if(model->UsesRuntimeMaterialGraph()) {
					if(auto material = model->GetMaterialAsset()) {
						CalyxEngine::MaterialGraphRuntimeShader shader = runtimeMaterialShaderCache_.GetOrCompileObject3DPixelShader(*material);
						if(shader.pixelShader) {
							const auto generatedSet = psoService->GetGeneratedMaterialSkinnedPipelineSet(model->GetBlendMode(), shader.pixelShader, shader.hash);
							psoService->SetCommand(generatedSet, cmdList);
							if(shadowMapSystem) {
								shadowMapSystem->BindForMainPass(cmdList);
							}
							BindRaytracingScene(cmdList);
							if(auto* cam = CameraManager::GetActive()) {
								cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
							} else {
								continue;
							}
							lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
							usingGeneratedPipeline = true;
						} else if(usingGeneratedPipeline) {
							const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
							psoService->SetCommand(ps, cmdList);
							if(shadowMapSystem) {
								shadowMapSystem->BindForMainPass(cmdList);
							}
							BindRaytracingScene(cmdList);
							if(auto* cam = CameraManager::GetActive()) {
								cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
							} else {
								continue;
							}
							lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
							usingGeneratedPipeline = false;
						}
					}
				} else if(usingGeneratedPipeline) {
					const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
					psoService->SetCommand(ps, cmdList);
					if(shadowMapSystem) {
						shadowMapSystem->BindForMainPass(cmdList);
					}
					BindRaytracingScene(cmdList);
					if(auto* cam = CameraManager::GetActive()) {
						cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
					} else {
						continue;
					}
					lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
					usingGeneratedPipeline = false;
				}

				const UINT need = static_cast<UINT>(visible.size());
				model->EnsureInstanceCapacity(device, need);
				model->UploadInstanceMatrices(visible);
				cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());

				if(model->UsesRuntimeMaterialGraph()) {
					model->TransferMaterial();
				}
				model->BindMaterialCB(cmdList);
				cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());
				cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(0));
				cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());
				model->SetCommandPalletSrv(7, cmdList);

				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				model->BindVertexIndexBuffers(cmdList);

				const auto& meshResource = model->GetModelData()->meshResource;
				const auto& subMeshes = meshResource.SubMeshes();
				if(subMeshes.empty()) {
					const UINT indexCount = static_cast<UINT>(meshResource.Indices().size());
					cmdList->DrawIndexedInstanced(indexCount, need, 0, 0, 0);
				} else {
					for(const auto& subMesh : subMeshes) {
						cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv(subMesh.materialIndex));
						cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(subMesh.materialIndex));
						cmdList->DrawIndexedInstanced(subMesh.indexCount, need, subMesh.indexStart, 0, 0);
					}
				}
			}
		}
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
//		Picking / Outline / IDPass 用 可視リスト収集
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::CollectVisibleStatic(std::vector<RenderInstance>& out) const {
	for(const auto& [model, insts] : staticModels_) {
		if(!model) continue;

		for(const auto& inst : insts) {
			if(!inst.visible) continue;
			if(!inst.owner) continue;

			out.push_back(RenderInstance{
				model,
				&inst.tf,
				inst.owner,
				inst.mode});
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//		Picking / Outline / IDPass 用 可視リスト収集
///////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::CollectVisibleSkinned(std::vector<RenderInstance>& out) const {
	for(const auto& [model, insts] : skinnedModels_) {
		if(!model) continue;

		for(const auto& inst : insts) {
			if(!inst.visible) continue;
			if(!inst.owner) continue;

			out.push_back(RenderInstance{
				model,
				&inst.tf,
				inst.owner,
				BillboardMode::None});
		}
	}
}
