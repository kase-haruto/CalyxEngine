#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/Objects/Transform/Transform.h>

#include <unordered_map>
#include <vector>
#include <d3d12.h>

class BaseModel;
class AnimationModel;
class Camera3d;
class LightLibrary;

class ModelRenderer{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	void RegisterStatic(BaseModel* model, const WorldTransform& transform);
	void RegisterSkinned(AnimationModel* model, const WorldTransform& transform);
	void Clear();
	void BeginFrame();
	void PreCullAndBatch(const Camera3d* camera);

	void DrawAll(ID3D12GraphicsCommandList* cmdList,
				 ID3D12Device* device,
				 const class Camera3d* camera,
				 class PipelineService* psoService,
				 class LightLibrary* lightLibrary);

	void MarkStaticDirty(BaseModel* model, size_t index);
	void MarkSkinnedDirty(AnimationModel* model, size_t index);

private:
	//===================================================================*/
	//					private types
	//===================================================================*/
	struct InstanceStatic{
		WorldTransform tf;
		AABB worldAABB {};
		bool dirty = true;
		bool visible = false;
	};
	struct InstanceSkinned{
		WorldTransform tf;
		AABB worldAABB {};
		bool dirty = true;
		bool visible = false;
	};

	using PipelineKey = PipelineService::PipelineKey;
	using PipelineKeyHasher = PipelineService::PipelineKeyHasher;
	using StaticBatch = std::vector<std::pair<BaseModel*, std::vector<WorldTransform>>>;
	using SkinnedBatch = std::vector<std::pair<AnimationModel*, std::vector<WorldTransform>>>;

	//===================================================================*/
	//					private methods
	//===================================================================*/
	void BuildStaticBatches();
	void BuildSkinnedBatches();

	//===================================================================*/
	//					private fields
	//===================================================================*/
	std::unordered_map<BaseModel*, std::vector<InstanceStatic>>   staticModels_;
	std::unordered_map<AnimationModel*, std::vector<InstanceSkinned>>  skinnedModels_;

	std::unordered_map<PipelineKey, StaticBatch, PipelineKeyHasher> staticBatches_;
	std::unordered_map<PipelineKey, SkinnedBatch, PipelineKeyHasher> skinnedBatches_;

	std::vector<WorldTransform> tempVisibleStatic_;
	std::vector<WorldTransform> tempVisibleSkinned_;
};