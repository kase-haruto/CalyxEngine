#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Shadow/Raytracing/RaytracingScene.h>
#include <Engine/Graphics/Shadow/Raytracing/RaytracingSystem.h>

#include <unordered_map>
#include <vector>
#include <d3d12.h>

class BaseModel;
class Camera3d;
class LightLibrary;

namespace CalyxAssets {
	class AnimationModel;
}

class ModelRenderer {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	ModelRenderer();
	void RegisterStatic(BaseModel* model,const WorldTransform& transform,BillboardMode billMode);
	void RegisterSkinned(CalyxAssets::AnimationModel* model,const WorldTransform& transform);
	void Clear();
	void BeginFrame();
	void PreCullAndBatch(const Camera3d* camera);

	void DrawAll(ID3D12GraphicsCommandList* cmdList,
				 ID3D12Device*              device,
				 const class Camera3d*      camera,
				 class PipelineService*     psoService,
				 class LightLibrary*        lightLibrary);

	void MarkStaticDirty(BaseModel* model,size_t index);
	void MarkSkinnedDirty(CalyxAssets::AnimationModel* model,size_t index);

private:
	//===================================================================*/
	//					private types
	//===================================================================*/
	struct InstanceStatic {
		WorldTransform tf;
		AABB           worldAABB{};
		bool           dirty   = true;
		bool           visible = false;
		BillboardMode  mode    = BillboardMode::None; //< 初期は無効
	};

	struct InstanceSkinned {
		WorldTransform tf;
		AABB           worldAABB{};
		bool           dirty   = true;
		bool           visible = false;
	};

	using PipelineKey       = PipelineService::PipelineKey;
	using PipelineKeyHasher = PipelineService::PipelineKeyHasher;

	struct StaticBatchItem {
		BaseModel*                             model = nullptr;
		std::vector<WorldTransform>            transforms; // t0 用
		std::vector<GpuBillboardParams>        billboards; // t1 用
		DxStructuredBuffer<GpuBillboardParams> billboardSrv;
	};

	using StaticBatch  = std::vector<StaticBatchItem>;
	using SkinnedBatch = std::vector<std::pair<CalyxAssets::AnimationModel*,std::vector<WorldTransform>>>;

	//===================================================================*/
	//					private methods
	//===================================================================*/
	void BuildStaticBatches();
	void BuildSkinnedBatches();

	//===================================================================*/
	//					private fields
	//===================================================================*/
	std::unordered_map<BaseModel*,std::vector<InstanceStatic>>                    staticModels_;
	std::unordered_map<CalyxAssets::AnimationModel*,std::vector<InstanceSkinned>> skinnedModels_;

	std::unordered_map<PipelineKey,StaticBatch,PipelineKeyHasher>  staticBatches_;
	std::unordered_map<PipelineKey,SkinnedBatch,PipelineKeyHasher> skinnedBatches_;

	std::vector<WorldTransform> tempVisibleStatic_;
	std::vector<WorldTransform> tempVisibleSkinned_;


};