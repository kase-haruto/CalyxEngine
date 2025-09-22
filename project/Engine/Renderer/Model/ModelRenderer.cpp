#include "ModelRenderer.h"

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Lighting/LightLibrary.h>

#include "Engine/Graphics/Context/GraphicsGroup.h"

ModelRenderer::ModelRenderer() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル登録（ビルボードモード付き）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterStatic(BaseModel* model, const WorldTransform& transform, BillboardMode billMode){
	InstanceStatic inst {};
	inst.tf      = transform;
	inst.dirty   = true;
	inst.visible = false;
	inst.mode    = billMode;
	staticModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーションモデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterSkinned(AnimationModel* model, const WorldTransform& transform){
	InstanceSkinned inst {};
	inst.tf      = transform;
	inst.dirty   = true;
	inst.visible = false;
	skinnedModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		クリア
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::Clear(){
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
void ModelRenderer::BeginFrame(){
	for (auto& [m, insts] : staticModels_){
		for (auto& inst : insts){ inst.visible = false; }
	}
	for (auto& [m, insts] : skinnedModels_){
		for (auto& inst : insts){ inst.visible = false; }
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
void ModelRenderer::MarkStaticDirty(BaseModel* model, size_t index){
	auto it = staticModels_.find(model);
	if (it == staticModels_.end()) return;
	if (index >= it->second.size()) return;
	it->second[index].dirty = true;
}

void ModelRenderer::MarkSkinnedDirty(AnimationModel* model, size_t index){
	auto it = skinnedModels_.find(model);
	if (it == skinnedModels_.end()) return;
	if (index >= it->second.size()) return;
	it->second[index].dirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台判定 & バッチ化
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::PreCullAndBatch(const Camera3d* camera){
	// -------------------- 静的モデル --------------------
	for (auto& [model, insts] : staticModels_){
		if (!model->GetModelData().has_value() || !model->GetIsDrawEnable()) continue;
		const AABB& local = model->GetModelData()->localAABB;

		for (auto& inst : insts){
			if (inst.dirty){
				inst.worldAABB = local.Transform(inst.tf.matrix.world);
				inst.dirty = false;
			}
			inst.visible = camera->IsVisible(inst.worldAABB);
		}
	}

	// -------------------- スキンモデル --------------------
	for (auto& [model, insts] : skinnedModels_){
		if (!model->GetModelData().has_value()) continue;
		const AABB& local = model->GetModelData()->localAABB;

		for (auto& inst : insts){
			if (inst.dirty){
				inst.worldAABB = local.Transform(inst.tf.matrix.world);
				inst.dirty = false;
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
void ModelRenderer::BuildStaticBatches(){
	for (auto& [model, insts] : staticModels_){
		if (!model->GetModelData().has_value() || !model->GetIsDrawEnable()) continue;

		std::vector<WorldTransform>     visTf;
		std::vector<GpuBillboardParams> visBb;
		visTf.reserve(insts.size());
		visBb.reserve(insts.size());

		for (auto& inst : insts){
			if (!inst.visible) continue;
			visTf.push_back(inst.tf);

			GpuBillboardParams p{};
			p.mode = static_cast<uint32_t>(inst.mode);
			visBb.push_back(p);
		}
		if (visTf.empty()) continue;

		PipelineKey key {PipelineTag::Object::Object3d, model->GetBlendMode()};
		auto& batch = staticBatches_[key];

		StaticBatchItem item;
		item.model = model;
		item.transforms.swap(visTf);
		item.billboards.swap(visBb);
		batch.emplace_back(std::move(item));
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		スキンモデル・バッチ作成（従来通り）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildSkinnedBatches(){
	for (auto& [model, insts] : skinnedModels_){
		if (!model->GetModelData().has_value()) continue;

		tempVisibleSkinned_.clear();
		tempVisibleSkinned_.reserve(insts.size());

		for (auto& inst : insts){
			if (inst.visible){
				tempVisibleSkinned_.push_back(inst.tf);
			}
		}
		if (tempVisibleSkinned_.empty()) continue;

		PipelineKey key {PipelineTag::Object::SkinningObject3D, model->GetBlendMode()};
		auto& batch = skinnedBatches_[key];
		batch.emplace_back(model, std::vector<WorldTransform>());
		batch.back().second.swap(tempVisibleSkinned_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		一斉描画（静的：t1 に Billboard SRV をバインド）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::DrawAll(ID3D12GraphicsCommandList* cmdList,
							ID3D12Device* device,
							[[maybe_unused]] const Camera3d* /*unused*/,
							PipelineService* psoService,
							LightLibrary* lightLibrary){
	psoService->ResetState();

	//------------------------------------------------------------
	// 1) 静的モデル
	//------------------------------------------------------------
	{
		PipelineKey lastKey {};
		bool        hasLast = false;

		for (auto& [key, batch] : staticBatches_){
			if (batch.empty()) continue;

			if (!hasLast || !(key == lastKey)){
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				if (auto* cam = CameraManager::GetActive())
					cam->SetCommand(cmdList, PipelineType::Object3D);

				lightLibrary->SetCommand(cmdList, PipelineType::Object3D);

				lastKey = key;
				hasLast = true;
			}

			for (auto& item : batch){
				auto* model   = item.model;
				auto& visible = item.transforms;
				if (visible.empty()) continue;

				// --- Billboard SRV 準備（Upload ヒープ / 可視数に合わせて） ---
				const UINT need = static_cast<UINT>(item.billboards.size());
				if (!billboardBuf_.IsValid() || billboardBuf_.GetElementCount() < need){
					billboardBuf_.ReleaseSrv();
					billboardBuf_.Reset();
					billboardBuf_.Initialize(device, need); // Upload
					billboardBuf_.CreateSrv(device);        // t1 用 SRV
				}
				// 書き込み
				std::memcpy(billboardBuf_.Data(),
							item.billboards.data(),
							sizeof(GpuBillboardParams) * need);

				// ★ gBillboard(t1) を含む SRV テーブルをセット（要：RootParam index合わせ）
				cmdList->SetGraphicsRootDescriptorTable(kBillboardSrvRootSlot_Object3D,
				                                        billboardBuf_.GetGpuSrvHandle());

				// インスタンシング描画（VS で gTransMat[t0], gBillboard[t1] を SV_InstanceID で参照）
				model->DrawInstanced(visible, cmdList);
			}
		}
	}

	//------------------------------------------------------------
	// 2) スキンメッシュ（従来通り）
	//------------------------------------------------------------
	{
		PipelineKey lastKey {};
		bool        hasLast = false;

		for (auto& [key, batch] : skinnedBatches_){
			if (batch.empty()) continue;

			if (!hasLast || !(key == lastKey)){
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				if (auto* cam = CameraManager::GetActive())
					cam->SetCommand(cmdList, PipelineType::SkinningObject3D);

				lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);

				lastKey = key;
				hasLast = true;
			}

			for (auto& [model, visible] : batch){
				for (const auto& tf : visible) model->Draw(tf);
			}
		}
	}
}