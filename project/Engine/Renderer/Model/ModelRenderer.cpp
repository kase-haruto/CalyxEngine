#include "ModelRenderer.h"

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


ModelRenderer::ModelRenderer() {
	Microsoft::WRL::ComPtr<ID3D12Device5> device5;
	GraphicsGroup::GetInstance()->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5));

	if(device5) {
		raytracingSystem_ = std::make_unique<CalyxGraphics::RaytracingSystem>();
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
void ModelRenderer::RegisterSkinned(CalyxAssets::AnimationModel* model, const WorldTransform& transform, SceneObject* owner) {
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

void ModelRenderer::MarkSkinnedDirty(CalyxAssets::AnimationModel* model, size_t index) {
	auto it = skinnedModels_.find(model);
	if(it == skinnedModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台判定 & バッチ化
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::PreCullAndBatch(const Camera3d* camera) {

	// =========================================================
	// Shadow 用 visible / SceneBounds 初期化
	// =========================================================
	staticVisibleForShadow_.clear();
	skinnedVisibleForShadow_.clear();

	// Raytracing Scene Clear
	raytracingScene_.Clear();

	hasSceneBounds_ = false;

	auto ExpandSceneBounds = [&](const AABB& aabb) {
		if(!hasSceneBounds_) {
			sceneBounds_	= aabb;
			hasSceneBounds_ = true;
			return;
		}
		sceneBounds_.min_ = CalyxMath::Vector3::Min(sceneBounds_.min_, aabb.min_);
		sceneBounds_.max_ = CalyxMath::Vector3::Max(sceneBounds_.max_, aabb.max_);
	};

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
			// ShadowPass：無条件で登録 (影を落とす場合のみ)
			// -------------------------
			if(!inst.owner || inst.owner->IsCastShadow()) {
				staticVisibleForShadow_[model].push_back(inst.tf);
				ExpandSceneBounds(inst.worldAABB);
			}

			// -------------------------
			// MainPass：カメラカリング
			// -------------------------
			inst.visible = camera->IsVisible(inst.worldAABB);

			inst.visible = camera->IsVisible(inst.worldAABB);
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
			// ShadowPass：無条件で登録 (影を落とす場合のみ)
			// -------------------------
			if(!inst.owner || inst.owner->IsCastShadow()) {
				skinnedVisibleForShadow_[model].push_back(inst.tf);
				ExpandSceneBounds(inst.worldAABB);
			}

			// -------------------------
			// MainPass：カメラカリング
			// -------------------------
			inst.visible = camera->IsVisible(inst.worldAABB);

			inst.visible = camera->IsVisible(inst.worldAABB);
		}
	}

	// =========================================================
	// MainPass 用バッチ生成
	// =========================================================
	BuildStaticBatches();
	BuildSkinnedBatches();
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

		StaticBatchItem item;
		item.model = model;
		item.transforms.swap(visTf);
		item.billboards.swap(visBb);
		batch.emplace_back(std::move(item));
	}
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
							CalyxGraphics::ShadowMapSystem* shadowMapSystem) {

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
							CalyxMath::Matrix3x4::ToMatrix3x4(tf.matrix.world),
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
							CalyxMath::Matrix3x4::ToMatrix3x4(tf.matrix.world),
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

		for(auto& [key, batch] : staticBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				shadowMapSystem->BindForMainPass(cmdList);

				if(raytracingSystem_) {
					cmdList->SetGraphicsRootShaderResourceView(
						10, // Space0, t3
						raytracingSystem_->GetTLAS()->GetGPUVirtualAddress());
				}

				if(auto* cam = CameraManager::GetActive()) {
					cam->SetCommand(cmdList, PipelineType::Object3D);
				} else {
					// 判定漏れ防止
					continue;
				}

				lightLibrary->SetCommand(cmdList, PipelineType::Object3D);

				lastKey = key;
				hasLast = true;
			}

			for(auto& item : batch) {
				BaseModel* model   = item.model;
				auto&	   visible = item.transforms;
				if(!model || visible.empty()) continue;

				const UINT need = static_cast<UINT>(item.billboards.size());
				if(need == 0) continue;
				assert(item.transforms.size() == item.billboards.size());

				model->EnsureBillboardCapacity(device, need);
				model->UploadBillboardParams(item.billboards);
				cmdList->SetGraphicsRootDescriptorTable(7, model->GetBillboardSrv());

				model->EnsureInstanceCapacity(device, need);
				model->UploadInstanceMatrices(visible);
				cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());

				model->BindMaterialCB(cmdList);
				cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());
				cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());

				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				model->BindVertexIndexBuffers(cmdList);

				const UINT indexCount = static_cast<UINT>(model->GetModelData()->meshResource.Indices().size());
				cmdList->DrawIndexedInstanced(indexCount, need, 0, 0, 0);
			}
		}
	}

	//------------------------------------------------------------
	// スキンメッシュ
	//------------------------------------------------------------
	{
		PipelineKey lastKey{};
		bool		hasLast = false;

		for(auto& [key, batch] : skinnedBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				shadowMapSystem->BindForMainPass(cmdList);

				if(raytracingSystem_) {
					cmdList->SetGraphicsRootShaderResourceView(
						10, // Space0, t3
						raytracingSystem_->GetTLAS()->GetGPUVirtualAddress());
				}

				if(auto* cam = CameraManager::GetActive()) {
					cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
				} else {
					// 判定漏れ防止
					continue;
				}

				lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);

				lastKey = key;
				hasLast = true;
			}

			for(auto& [model, visible] : batch) {
				for(const auto& tf : visible) model->Draw(tf);
			}
		}
	}

#if defined(_DEBUG) || defined(DEVELOP)
	// debugViewでのみ描画
	if(rt->GetRenderTargetType() != RenderTargetType::DebugView) return;

	//------------------------------------------------------------
	// 選択オブジェクトのワイヤーフレーム（オレンジ）描画
	//------------------------------------------------------------
	if(auto* ctx = SceneContext::Current()) {
		if(auto* selected = ctx->GetDebugSelectedObject()) {

			// Static Models
			for(auto& [model, insts] : staticModels_) {
				if(!model->GetModelData() || !model->GetIsDrawEnable()) continue;

				std::vector<WorldTransform>		selectedTf;
				std::vector<GpuBillboardParams> selectedBb;
				for(auto& inst : insts) {
					if(inst.visible && inst.owner == selected) {
						selectedTf.push_back(inst.tf);
						GpuBillboardParams p{};
						p.mode = static_cast<uint32_t>(inst.mode);
						selectedBb.push_back(p);
					}
				}

				if(!selectedTf.empty()) {
					const auto ps = psoService->GetPipelineSet(PipelineTag::Object::WireframeObject3D, model->GetBlendMode());
					psoService->SetCommand(ps, cmdList);

					float thickness = 1.5f;
					cmdList->SetGraphicsRoot32BitConstants(12, 1, &thickness, 0);

					if(auto* cam = CameraManager::GetActive()) {
						cam->SetCommand(cmdList, PipelineType::Object3D);
					}
					lightLibrary->SetCommand(cmdList, PipelineType::Object3D);

					auto					 oldColor		 = model->GetColor();
					auto					 oldLighting	 = model->GetLightingMode();
					const CalyxMath::Vector4 orangeWireframe = {1.0f, 0.5f, 0.0f, 1.0f};
					model->SetColor(orangeWireframe);
					model->SetLightingMode(LightingMode::UnlitColor);
					model->TransferMaterial();

					const UINT need = static_cast<UINT>(selectedTf.size());
					model->EnsureBillboardCapacity(device, need);
					model->UploadBillboardParams(selectedBb);
					cmdList->SetGraphicsRootDescriptorTable(7, model->GetBillboardSrv());

					model->EnsureInstanceCapacity(device, need);
					model->UploadInstanceMatrices(selectedTf);
					cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());

					model->BindMaterialCB(cmdList);
					cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());
					cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());

					cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					model->BindVertexIndexBuffers(cmdList);

					cmdList->DrawIndexedInstanced(static_cast<UINT>(model->GetModelData()->meshResource.Indices().size()), need, 0, 0, 0);

					model->SetColor(oldColor);
					model->SetLightingMode(oldLighting);
					model->TransferMaterial();
				}
			}

			// Skinned Models
			for(auto& [model, insts] : skinnedModels_) {
				if(!model->GetModelData() || !model->GetIsDrawEnable()) continue;

				std::vector<WorldTransform> selectedTf;
				for(auto& inst : insts) {
					if(inst.visible && inst.owner == selected) {
						selectedTf.push_back(inst.tf);
					}
				}

				if(!selectedTf.empty()) {
					const auto ps = psoService->GetPipelineSet(PipelineTag::Object::WireframeSkinnedObject3D, model->GetBlendMode());
					psoService->SetCommand(ps, cmdList);

					float thickness = 2.0f;
					cmdList->SetGraphicsRoot32BitConstants(12, 1, &thickness, 0);

					if(auto* cam = CameraManager::GetActive()) {
						cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
					}
					lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);

					auto					 oldColor		 = model->GetColor();
					auto					 oldLighting	 = model->GetLightingMode();
					const CalyxMath::Vector4 orangeWireframe = {1.0f, 0.5f, 0.0f, 1.0f};
					model->SetColor(orangeWireframe);
					model->SetLightingMode(LightingMode::UnlitColor);
					model->TransferMaterial();

					for(const auto& tf : selectedTf) {
						// Set transforms again and draw
						model->UploadInstanceMatrices({tf});
						cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());
						model->BindMaterialCB(cmdList);
						cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());
						cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());

						cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
						model->BindVertexIndexBuffers(cmdList);
						cmdList->DrawIndexedInstanced(static_cast<UINT>(model->GetModelData()->meshResource.Indices().size()), 1, 0, 0, 0);
					}

					model->SetColor(oldColor);
					model->SetLightingMode(oldLighting);
					model->TransferMaterial();
				}
			}
		}
	}
#else
	(void)rt;
#endif
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
				inst.owner});
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
				inst.owner});
		}
	}
}