#include "LightLibrary.h"

#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include "Engine/Scene/Context/SceneContext.h"
#include "Engine/Scene/Utility/SceneUtility.h"

#include <algorithm>

/* GPU へ定数バッファ反映 ----------------------------------------------*/
void LightLibrary::CyncGpu() {
	if(directionalLight_) {
		// CPU側Light設定をフレーム描画前にConstant Bufferへ反映する。
		directionalLight_->UploadToGpu();
	}else {
		// SceneにDirectional Lightがない場合もShaderの必須Bindingを満たす既定Lightを生成する。
		directionalLight_ = SceneAPI::Instantiate<DirectionalLight>("DirectionalLight");
	}

	// 失効参照を除去してから固定長GPU配列を再構築し、前フレームのLightを残さない。
	EnsurePointLightBuffer();
	CleanupPointLights();

	pointLightConstants_ = {};
	// Editor設定をGPU Flagへ変換し、Shader側でPoint Shadow処理を分岐できるようにする。
	const auto& graphicsSettings = CalyxEngine::EngineSettings::GetInstance()->GetData().graphics;
	pointLightConstants_.pointLightShadowsEnabled = graphicsSettings.enablePointLightShadows ? 1u : 0u;
	pointLightConstants_.maxPointShadowLights = 2u;
	pointLightConstants_.pointShadowContributionThreshold = 0.01f;
	uint32_t writeIndex = 0;
	// Shaderの固定上限まで有効Lightを詰め、超過分は次回設定変更までCPU側に保持する。
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
	// Libraryはweak_ptrだけを保持し、SceneObjectのLifetimeを延長しない。
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
	// Scene破棄時にCPU参照とGPU定数の両方を空へ同期し、旧SceneのLightingを残さない。
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
	// Point Light用Root Parameterを持つ3D PipelineにだけBufferをBindingする。
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
	// GPU Bufferは最初のLight同期時に遅延生成し、Lighting未使用Sceneの確保を避ける。
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
