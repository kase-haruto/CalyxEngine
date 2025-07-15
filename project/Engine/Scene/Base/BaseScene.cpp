#include "BaseScene.h"
/* ========================================================================
/*	include space
/* ===================================================================== */
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Graphics/Pipeline/Presets/PipelinePresets.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Application/Effects/FxSystem.h>

BaseScene::BaseScene(){
	sceneContext_ = std::make_unique<SceneContext>();

	
}

void BaseScene::Draw(ID3D12GraphicsCommandList* cmdList,
					 PipelineService* psoService,
					 RenderTargetType renderTargetType){
	sceneContext_->Render(cmdList, psoService, renderTargetType);
}
