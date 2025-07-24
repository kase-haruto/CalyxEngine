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
	// インスタンス登録
	void RegisterStatic(BaseModel* model, const WorldTransform& transform);
	void RegisterSkinned(AnimationModel* model, const WorldTransform& transform);

	// 全削除
	void Clear();

	// フレーム開始時に呼ぶ（可視フラグなどを初期化）
	void BeginFrame();

	// 視錐台判定とバッチ化を事前に実行
	void PreCullAndBatch(const Camera3d* camera);

	// 描画
	void DrawAll(ID3D12GraphicsCommandList* cmdList,
				 ID3D12Device* device,
				 const class Camera3d* camera,
				 class PipelineService* psoService,
				 class LightLibrary* lightLibrary);

	// Transform を更新したときに呼ぶと AABB を再計算（dirty）させる
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

	// バッチ用マップ（毎フレーム clearのみ）
	std::unordered_map<PipelineKey, StaticBatch, PipelineKeyHasher> staticBatches_;
	std::unordered_map<PipelineKey, SkinnedBatch, PipelineKeyHasher> skinnedBatches_;

	// 可視Transform一時格納用（reserve 再利用）
	std::vector<WorldTransform> tempVisibleStatic_;
	std::vector<WorldTransform> tempVisibleSkinned_;
};