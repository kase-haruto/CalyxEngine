#include "ModelRenderer.h"

/* ========================================================================
/* include space
/* ===================================================================== */
#include "Engine/Foundation/Math/Matrix3x4.h"

#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Lighting/LightLibrary.h>

#include "Engine/Graphics/Context/GraphicsGroup.h"

ModelRenderer::ModelRenderer() {
	Microsoft::WRL::ComPtr<ID3D12Device5> device5;
	GraphicsGroup::GetInstance()->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5));

}

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル登録（ビルボードモード付き）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterStatic(BaseModel* model,const WorldTransform& transform,BillboardMode billMode) {
	InstanceStatic inst{};
	inst.tf      = transform;
	inst.dirty   = true;
	inst.visible = false;
	inst.mode    = billMode;
	staticModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーションモデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterSkinned(CalyxAssets::AnimationModel* model,const WorldTransform& transform) {
	InstanceSkinned inst{};
	inst.tf      = transform;
	inst.dirty   = true;
	inst.visible = false;
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
	for(auto& insts : staticModels_ | std::views::values) { for(auto& inst : insts) { inst.visible = false; } }
	for(auto& insts : skinnedModels_ | std::views::values) { for(auto& inst : insts) { inst.visible = false; } }
	staticBatches_.clear();
	skinnedBatches_.clear();

	// 毎フレ登録方式なのでクリア
	staticModels_.clear();
	skinnedModels_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		Dirty マーク
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::MarkStaticDirty(BaseModel* model,size_t index) {
	auto it = staticModels_.find(model);
	if(it == staticModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

void ModelRenderer::MarkSkinnedDirty(CalyxAssets::AnimationModel* model,size_t index) {
	auto it = skinnedModels_.find(model);
	if(it == skinnedModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台判定 & バッチ化
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::PreCullAndBatch(const Camera3d* camera) {
	// -------------------- 静的モデル --------------------
	for(auto& [model, insts] : staticModels_) {
		if(!model->GetModelData().has_value() || !model->GetIsDrawEnable()) continue;
		const AABB& local = model->GetModelData()->localAABB;

		for(auto& inst : insts) {
			if(inst.dirty) {
				inst.worldAABB = local.Transform(inst.tf.matrix.world);
				inst.dirty     = false;
			}
			inst.visible = camera->IsVisible(inst.worldAABB);
		}
	}

	// -------------------- スキンモデル --------------------
	for(auto& [model, insts] : skinnedModels_) {
		if(!model->GetModelData().has_value()) continue;
		const AABB& local = model->GetModelData()->localAABB;

		for(auto& inst : insts) {
			if(inst.dirty) {
				inst.worldAABB = local.Transform(inst.tf.matrix.world);
				inst.dirty     = false;
			}
			inst.visible = camera->IsVisible(inst.worldAABB);
		}
	}

	BuildStaticBatches();
	BuildSkinnedBatches();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル・バッチ作成（BillboardParams も可視分だけ詰める）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildStaticBatches() {
	for(auto& [model, insts] : staticModels_) {
		if(!model->GetModelData().has_value() || !model->GetIsDrawEnable()) continue;

		std::vector<WorldTransform>     visTf;
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

		PipelineKey key{PipelineTag::Object::Object3d,model->GetBlendMode()};
		auto&       batch = staticBatches_[key];

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
		if(!model->GetModelData().has_value()) continue;

		tempVisibleSkinned_.clear();
		tempVisibleSkinned_.reserve(insts.size());

		for(auto& inst : insts) { if(inst.visible) { tempVisibleSkinned_.push_back(inst.tf); } }
		if(tempVisibleSkinned_.empty()) continue;

		PipelineKey key{PipelineTag::Object::SkinningObject3D,model->GetBlendMode()};
		auto&       batch = skinnedBatches_[key];
		batch.emplace_back(model,std::vector<WorldTransform>());
		batch.back().second.swap(tempVisibleSkinned_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		一斉描画
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::DrawAll(ID3D12GraphicsCommandList* cmdList,
							ID3D12Device*              device,
							[[maybe_unused]] const Camera3d* /*unused*/,
							PipelineService* psoService,
							LightLibrary*    lightLibrary) {
	psoService->ResetState();
	// // ============================================================
	// // Inline Raytracing : Build TLAS (Shadow)
	// // ============================================================
	// {
	// 	raytracingScene_.Clear();
	//
	// 	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> cmd4;
	// 	HRESULT                                            hr1 = cmdList->QueryInterface(IID_PPV_ARGS(&cmd4));
	// 	assert(SUCCEEDED(hr1) && cmd4);
	//
	// 	Microsoft::WRL::ComPtr<ID3D12Device5> device5;
	// 	HRESULT                               hr2 = device->QueryInterface(IID_PPV_ARGS(&device5));
	// 	assert(SUCCEEDED(hr2) && device5);
	//
	// 	// --- 静的モデル（可視インスタンスのみ） ---
	// 	uint32_t instanceID = 0;
	// 	for(auto& [model, insts] : staticModels_) {
	// 		if(!model || !model->GetModelData().has_value()) continue;
	// 		if(!model->GetIsDrawEnable()) continue;
	//
	// 		for(auto& inst : insts) {
	// 			if(!inst.visible) continue;
	//
	// 			// BLAS を必要なら構築（遅延構築）
	// 			model->EnsureRaytracingBLAS(device5.Get(),cmd4.Get());
	// 			if(!model->HasBLAS()) continue;
	//
	// 			raytracingScene_.AddInstance(
	// 				CalyxMath::Matrix3x4::ToMatrix3x4(inst.tf.matrix.world),
	// 				model->GetBLAS(),
	// 				instanceID++,
	// 				0xFF
	// 				);
	// 		}
	// 	}
	//
	// 	// InstanceDesc Upload
	// 	raytracingScene_.EnsureBuffer(device);
	// 	raytracingScene_.Upload();
	//
	// 	// TLAS Build
	// 	raytracingSystem_.BuildTLAS(cmd4.Get(),raytracingScene_);
	//
	// }

	// ------------------------------------------------------------
	// 静的モデル描画
	// ------------------------------------------------------------
	{
		PipelineKey lastKey{};
		bool        hasLast = false;

		for(auto& [key, batch] : staticBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag,key.blend);
				psoService->SetCommand(ps,cmdList);

				// // ここが「影が出る設定」で一番大事：TLAS SRV をPSへ渡す
				// // RootParameterIndex はあなたの RootSignature に合わせて変更
				// constexpr UINT kRoot_TLAS = 8;
				// cmdList->SetGraphicsRootDescriptorTable(kRoot_TLAS, raytracingSystem_.GetTLASSrv());

				if(auto* cam = CameraManager::GetActive())
					cam->SetCommand(cmdList,PipelineType::Object3D);

				lightLibrary->SetCommand(cmdList,PipelineType::Object3D);

				lastKey = key;
				hasLast = true;
			}

			for(auto& item : batch) {
				BaseModel* model   = item.model;
				auto&      visible = item.transforms;
				if(!model || visible.empty()) continue;

				const UINT need = static_cast<UINT>(item.billboards.size());
				if(need == 0) continue;
				assert(item.transforms.size() == item.billboards.size());

				model->EnsureBillboardCapacity(device,need);
				model->UploadBillboardParams(item.billboards);
				cmdList->SetGraphicsRootDescriptorTable(7,model->GetBillboardSrv());

				model->EnsureInstanceCapacity(device,need);
				model->UploadInstanceMatrices(visible);
				cmdList->SetGraphicsRootDescriptorTable(1,model->GetInstanceSrv());

				model->BindMaterialCB(cmdList);
				cmdList->SetGraphicsRootDescriptorTable(2,model->GetTexSrv());
				cmdList->SetGraphicsRootDescriptorTable(6,model->GetEnvMapSrv());

				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				model->BindVertexIndexBuffers(cmdList);

				const UINT indexCount = static_cast<UINT>(model->GetModelData()->meshData.indices.size());
				cmdList->DrawIndexedInstanced(indexCount,need,0,0,0);
			}
		}
	}

	//------------------------------------------------------------
	// スキンメッシュ
	//------------------------------------------------------------
	{
		PipelineKey lastKey{};
		bool        hasLast = false;

		for(auto& [key, batch] : skinnedBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag,key.blend);
				psoService->SetCommand(ps,cmdList);

				if(auto* cam = CameraManager::GetActive())
					cam->SetCommand(cmdList,PipelineType::SkinningObject3D);

				lightLibrary->SetCommand(cmdList,PipelineType::SkinningObject3D);

				lastKey = key;
				hasLast = true;
			}

			for(auto& [model, visible] : batch) { for(const auto& tf : visible) model->Draw(tf); }
		}
	}
}