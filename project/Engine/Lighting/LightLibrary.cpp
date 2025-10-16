#include "LightLibrary.h"

#include "Engine/Scene/Context/SceneContext.h"
#include "Engine/Scene/Utility/SceneUtility.h"

/* GPU へ定数バッファ反映 ----------------------------------------------*/
void LightLibrary::CyncGpu() {
	if(directionalLight_) {
		directionalLight_->UploadToGpu();
	}else {
		directionalLight_ = SceneAPI::Instantiate<DirectionalLight>("DirectionalLight");
	}
	if(pointLight_) pointLight_->UploadToGpu();
}

/* 登録 ---------------------------------------------------------------*/
void LightLibrary::SetDirectionalLight(const std::shared_ptr<DirectionalLight>& light) { directionalLight_ = light; }
void LightLibrary::SetPointLight(const std::shared_ptr<PointLight>& light) { pointLight_ = light; }

/* クリア -------------------------------------------------------------*/
void LightLibrary::Clear() {
	directionalLight_.reset();
	pointLight_.reset();
}

/* コマンド積み込み -----------------------------------------------------*/
void LightLibrary::SetCommand(ID3D12GraphicsCommandList* cmdList,PipelineType pipelineType) {
	if(directionalLight_) directionalLight_->SetCommand(cmdList,pipelineType);
	if(pointLight_) pointLight_->SetCommand(cmdList,pipelineType);
}

void LightLibrary::SetCommand(ID3D12GraphicsCommandList* cmdList,
                              PipelineType               pipelineType,
                              LightType                  lightType) {
	if(lightType == LightType::Directional && directionalLight_) {
		directionalLight_->SetCommand(cmdList,pipelineType);
	} else if(lightType == LightType::Point && pointLight_) {
		pointLight_->SetCommand(cmdList,pipelineType);
	}
}