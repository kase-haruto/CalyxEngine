#include "LightLibrary.h"

#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include "Engine/Scene/Context/SceneContext.h"
#include "Engine/Scene/Utility/SceneUtility.h"

#include <algorithm>

/* GPU へ定数バッファ反映 ----------------------------------------------*/
void LightLibrary::CyncGpu() {
	if(directionalLight_) {
		directionalLight_->UploadToGpu();
	}else {
		directionalLight_ = SceneAPI::Instantiate<DirectionalLight>("DirectionalLight");
	}

	EnsurePointLightBuffer();
	CleanupPointLights();

	pointLightConstants_ = {};
	const auto& graphicsSettings = CalyxEngine::EngineSettings::GetInstance()->GetData().graphics;
	pointLightConstants_.pointLightShadowsEnabled = graphicsSettings.enablePointLightShadows ? 1u : 0u;
	pointLightConstants_.maxPointShadowLights = 2u;
	pointLightConstants_.pointShadowContributionThreshold = 0.01f;
	uint32_t writeIndex = 0;
	for(auto& weak : pointLights_) {
		if(writeIndex >= kMaxPointLightCount) break;
		auto light = weak.lock();
		if(!light) continue;

		light->SyncPositionFromTransform();
		pointLightConstants_.lights[writeIndex] = light->GetLightData();
		++writeIndex;
	}
	pointLightConstants_.count = writeIndex;
	pointLightBuffer_.TransferData(pointLightConstants_);
}

/* 登録 ---------------------------------------------------------------*/
void LightLibrary::SetDirectionalLight(const std::shared_ptr<DirectionalLight>& light) { directionalLight_ = light; }
void LightLibrary::SetPointLight(const std::shared_ptr<PointLight>& light) {
	pointLights_.clear();
	AddPointLight(light);
}

void LightLibrary::AddPointLight(const std::shared_ptr<PointLight>& light) {
	if(!light || ContainsPointLight(light.get())) return;
	pointLights_.push_back(light);
}

void LightLibrary::RemovePointLight(const std::shared_ptr<PointLight>& light) {
	if(!light) return;
	const PointLight* raw = light.get();
	pointLights_.erase(
		std::remove_if(pointLights_.begin(), pointLights_.end(),
					   [raw](const std::weak_ptr<PointLight>& weak) {
						   auto sp = weak.lock();
						   return !sp || sp.get() == raw;
					   }),
		pointLights_.end());
}

/* クリア -------------------------------------------------------------*/
void LightLibrary::Clear() {
	directionalLight_.reset();
	pointLights_.clear();
	pointLightConstants_ = {};
	if(pointLightBuffer_.IsInitialized()) {
		pointLightBuffer_.TransferData(pointLightConstants_);
	}
}

/* コマンド積み込み -----------------------------------------------------*/
void LightLibrary::SetCommand(ID3D12GraphicsCommandList* cmdList,PipelineType pipelineType) {
	if(directionalLight_) directionalLight_->SetCommand(cmdList,pipelineType);
	if((pipelineType == PipelineType::Object3D ||
		pipelineType == PipelineType::SkinningObject3D) &&
	   pointLightBuffer_.IsInitialized()) {
		pointLightBuffer_.SetCommand(cmdList, 5);
	}
}

void LightLibrary::SetCommand(ID3D12GraphicsCommandList* cmdList,
                              PipelineType               pipelineType,
                              LightType                  lightType) {
	if(lightType == LightType::Directional && directionalLight_) {
		directionalLight_->SetCommand(cmdList,pipelineType);
	} else if(lightType == LightType::Point &&
			  (pipelineType == PipelineType::Object3D ||
			   pipelineType == PipelineType::SkinningObject3D) &&
			  pointLightBuffer_.IsInitialized()) {
		pointLightBuffer_.SetCommand(cmdList, 5);
	}
}

PointLight* LightLibrary::GetPointLight() const {
	for(const auto& weak : pointLights_) {
		if(auto light = weak.lock()) {
			return light.get();
		}
	}
	return nullptr;
}

void LightLibrary::EnsurePointLightBuffer() {
	if(pointLightBuffer_.IsInitialized()) return;
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	pointLightBuffer_.Initialize(device);
}

void LightLibrary::CleanupPointLights() {
	pointLights_.erase(
		std::remove_if(pointLights_.begin(), pointLights_.end(),
					   [](const std::weak_ptr<PointLight>& weak) {
						   return weak.expired();
					   }),
		pointLights_.end());
}

bool LightLibrary::ContainsPointLight(const PointLight* light) const {
	if(!light) return false;
	for(const auto& weak : pointLights_) {
		if(auto sp = weak.lock()) {
			if(sp.get() == light) return true;
		}
	}
	return false;
}
