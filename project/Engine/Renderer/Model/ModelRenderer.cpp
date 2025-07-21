#include "ModelRenderer.h"

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterStatic(BaseModel* model, const WorldTransform& transform){
	staticModels_[model].push_back(transform);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーションモデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterSkinned(AnimationModel* model, const WorldTransform& transform){
	skinnedModels_[model].push_back(transform);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		クリア
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::Clear(){
	staticModels_.clear();
	skinnedModels_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		イッセイ描画
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::DrawAll(ID3D12GraphicsCommandList* cmdList,
							[[maybe_unused]]ID3D12Device* device,
							const Camera3d* camera,
							PipelineService* psoService,
							LightLibrary* lightLibrary){

	// -------------------- 静的モデル描画 --------------------
	for (auto& [model, transforms] : staticModels_){
		std::vector<WorldTransform> visible;

		for (const auto& tf : transforms){
			if (!model->GetModelData().has_value()) continue;

			AABB worldAABB = model->GetModelData()->localAABB.Transform(tf.matrix.world);
			if (camera->IsVisible(worldAABB)){
				visible.push_back(tf);
			}
		}
		if (visible.empty()) continue;

		auto desc = PipelinePresets::MakeObject3D(model->GetBlendMode());

		psoService->SetCommand(desc, cmdList);
		CameraManager::SetCommand(cmdList, PipelineType::Object3D);
		lightLibrary->SetCommand(cmdList, PipelineType::Object3D);

		model->DrawInstanced(visible, cmdList);
	}

	// -------------------- アニメーションモデル描画 --------------------
	for (auto& [model, transforms] : skinnedModels_){
		for (const auto& tf : transforms){
			if (!model->GetModelData().has_value()) continue;

			AABB worldAABB = model->GetModelData()->localAABB.Transform(tf.matrix.world);
			if (!camera->IsVisible(worldAABB)) continue;

			auto desc = PipelinePresets::MakeSkinningObject3D(model->GetBlendMode());

			psoService->SetCommand(desc, cmdList);
			CameraManager::SetCommand(cmdList, PipelineType::SkinningObject3D);
			lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);

			model->Draw(tf); // 通常描画（インスタンシング不要）
		}
	}
}
