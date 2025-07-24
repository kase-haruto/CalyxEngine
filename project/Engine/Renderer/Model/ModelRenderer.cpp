#include "ModelRenderer.h"

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Lighting/LightLibrary.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterStatic(BaseModel* model, const WorldTransform& transform){
	// {transform} 初期化は避ける
	InstanceStatic inst {};
	inst.tf = transform;
	inst.dirty = true;
	inst.visible = false;
	staticModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーションモデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterSkinned(AnimationModel* model, const WorldTransform& transform){
	InstanceSkinned inst {};
	inst.tf = transform;
	inst.dirty = true;
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
		for (auto& inst : insts){
			inst.visible = false;
		}
	}
	for (auto& [m, insts] : skinnedModels_){
		for (auto& inst : insts){
			inst.visible = false;
		}
	}
	staticBatches_.clear();
	skinnedBatches_.clear();

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
//		静的モデル・バッチ作成
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildStaticBatches(){
	for (auto& [model, insts] : staticModels_){
		if (!model->GetModelData().has_value() || !model->GetIsDrawEnable()) continue;

		tempVisibleStatic_.clear();
		tempVisibleStatic_.reserve(insts.size());

		for (auto& inst : insts){
			if (inst.visible){
				tempVisibleStatic_.push_back(inst.tf);
			}
		}
		if (tempVisibleStatic_.empty()) continue;

		PipelineKey key {PipelineTag::Object::Object3d, model->GetBlendMode()};
		auto& batch = staticBatches_[key];
		batch.emplace_back(model, std::vector<WorldTransform>());
		batch.back().second.swap(tempVisibleStatic_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		スキンモデル・バッチ作成
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
//		イッセイ描画
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::DrawAll(ID3D12GraphicsCommandList* cmdList,
							[[maybe_unused]] ID3D12Device* device,
							[[maybe_unused]] const Camera3d* camera,
							PipelineService* psoService,
							LightLibrary* lightLibrary){
	psoService->ResetState();

	// -------------------- 静的モデル描画 --------------------
	{
		PipelineKey lastKey {};
		bool hasLast = false;

		for (auto& [key, batch] : staticBatches_){
			if (batch.empty()) continue;

			if (!hasLast || !(key == lastKey)){
				auto pipelineSet = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(pipelineSet, cmdList);
				CameraManager::SetCommand(cmdList, PipelineType::Object3D);
				lightLibrary->SetCommand(cmdList, PipelineType::Object3D);
				lastKey = key;
				hasLast = true;
			}

			for (auto& [model, visible] : batch){
				if (!visible.empty()){
					model->DrawInstanced(visible, cmdList);
				}
			}
		}
	}

	// -------------------- アニメーションモデル描画 --------------------
	{
		PipelineKey lastKey {};
		bool hasLast = false;

		for (auto& [key, batch] : skinnedBatches_){
			if (batch.empty()) continue;

			if (!hasLast || !(key == lastKey)){
				auto pipelineSet = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(pipelineSet, cmdList);
				CameraManager::SetCommand(cmdList, PipelineType::SkinningObject3D);
				lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
				lastKey = key;
				hasLast = true;
			}

			for (auto& [model, visible] : batch){
				for (const auto& tf : visible){
					model->Draw(tf);
				}
			}
		}
	}
}
