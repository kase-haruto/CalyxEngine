#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

#include <unordered_map>
#include <vector>
#include <utility>
#include <d3d12.h>

class BaseModel;
class AnimationModel;
class WorldTransform;

class ModelRenderer{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	// インスタンス登録
	void RegisterStatic(BaseModel* model, const WorldTransform& transform);
	void RegisterSkinned(AnimationModel* model, const WorldTransform& transform);

	void Clear();

	void DrawAll(ID3D12GraphicsCommandList* cmdList,
				 ID3D12Device* device,
				 const class Camera3d* camera,
				 class PipelineService* psoService,
				 class LightLibrary* lightLibrary);

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	std::unordered_map<BaseModel*, std::vector<WorldTransform>> staticModels_;
	std::unordered_map<AnimationModel*, std::vector<WorldTransform>> skinnedModels_;
};

