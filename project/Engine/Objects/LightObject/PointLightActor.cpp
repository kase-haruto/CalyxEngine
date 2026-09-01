#include "PointLightActor.h"

#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/LightObject/PointLight.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#if defined(_DEBUG) || defined(DEVELOP)
#include <externals/imgui/imgui.h>
#endif

PointLightActor::PointLightActor()
	: Actor() {
	SceneObject::SetName("PointLightActor", ObjectType::Light);
	SetCastShadow(false);
	EnsurePointLight();
}

PointLightActor::PointLightActor(const std::string& modelName, std::optional<std::string> objectName)
	: Actor(modelName, objectName) {
	SceneObject::SetName(objectName.value_or("PointLightActor"), ObjectType::Light);
	SetCastShadow(false);
	EnsurePointLight();
}

PointLightActor::~PointLightActor() {
	UnregisterPointLight();
}

void PointLightActor::Initialize() {
	// Actor本体とLightデータを分離したまま、現在SceneのLightLibraryへ描画用Lightを登録する。
	EnsurePointLight();
	if(pointLight_) {
		pointLight_->Initialize();
	}
	if(!IsTransient()) {
		RegisterPointLight();
	}
	SyncLightTransform();
}

void PointLightActor::AlwaysUpdate(float dt) {
	Actor::AlwaysUpdate(dt);
	SyncLightTransform();
}

void PointLightActor::Destroy() {
	// Actor破棄より先にLibraryの非所有参照を外し、次フレームのLighting参照を無効化する。
	UnregisterPointLight();
	Actor::Destroy();
}

void PointLightActor::DerivativeGui() {
#if defined(_DEBUG) || defined(DEVELOP)
	EnsurePointLight();
	if(!pointLight_) return;

	auto& data = const_cast<PointLightData&>(pointLight_->GetLightData());
	ImGui::SeparatorText("Point Light");
	GuiCmd::DragFloat3("Light Local Position", lightLocalPosition_);
	GuiCmd::ColorEdit4("Light Color", data.color);
	GuiCmd::SliderFloat("Light Intensity", data.intensity, 0.0f, 1.0f);
	GuiCmd::DragFloat("Light Radius", data.radius);
	GuiCmd::DragFloat("Light Decay", data.decay);
	SyncLightTransform();
#endif
}

void PointLightActor::SetDrawEnable(bool enable) {
	SceneObject::SetDrawEnable(enable);
}

void PointLightActor::SetName(const std::string& name) {
	SceneObject::SetName(name, ObjectType::Light);
}

void PointLightActor::ApplyDerivedConfigFromJson([[maybe_unused]] const nlohmann::json& root,
												 const nlohmann::json* derived) {
	if(!derived) return;
	EnsurePointLight();
	if(!pointLight_) return;

	// 欠落項目は既定値を維持し、旧Scene/Presetとの後方互換性を保つ。
	if(derived->contains("lightLocalPosition")) {
		derived->at("lightLocalPosition").get_to(lightLocalPosition_);
	}

	if(derived->contains("color")) {
		SetLightColor(derived->at("color").get<CalyxEngine::Vector4>());
	}
	if(derived->contains("intensity")) {
		SetLightIntensity(derived->at("intensity").get<float>());
	}
	if(derived->contains("radius")) {
		SetLightRadius(derived->at("radius").get<float>());
	}
	if(derived->contains("decay")) {
		SetLightDecay(derived->at("decay").get<float>());
	}

	// 読込後のActor配置とLocal Offsetから、実際に描画されるLight座標を再計算する。
	SyncLightTransform();
}

void PointLightActor::ExtractDerivedConfigToJson([[maybe_unused]] nlohmann::json& root,
												 nlohmann::json& derived) const {
	if(!pointLight_) return;

	// TransientなPointLight Object自体は保存せず、Actorから再構築できる設定だけを抽出する。
	const PointLightData& data = pointLight_->GetLightData();
	derived["lightLocalPosition"] = lightLocalPosition_;
	derived["color"]			  = data.color;
	derived["intensity"]		  = data.intensity;
	derived["radius"]			  = data.radius;
	derived["decay"]			  = data.decay;
}

void PointLightActor::SetLightLocalPosition(const CalyxEngine::Vector3& localPosition) {
	lightLocalPosition_ = localPosition;
	SyncLightTransform();
}

void PointLightActor::SetLightColor(const CalyxEngine::Vector4& color) {
	EnsurePointLight();
	if(pointLight_) pointLight_->SetColor(color);
}

void PointLightActor::SetLightIntensity(float intensity) {
	EnsurePointLight();
	if(pointLight_) pointLight_->SetIntensity(intensity);
}

void PointLightActor::SetLightRadius(float radius) {
	EnsurePointLight();
	if(pointLight_) pointLight_->SetRadius(radius);
}

void PointLightActor::SetLightDecay(float decay) {
	EnsurePointLight();
	if(pointLight_) pointLight_->SetDecay(decay);
}

void PointLightActor::EnsurePointLight() {
	if(pointLight_) return;

	// 補助LightはActorが共有所有し、Sceneの通常Object一覧やPicking対象には含めない。
	pointLight_ = std::make_shared<PointLight>("PointLight");
	pointLight_->SetTransient(true);
	pointLight_->SetEnablePicking(false);
	pointLight_->SetCastShadow(false);
}

void PointLightActor::RegisterPointLight() {
	if(!pointLight_) return;
	// 現在ContextのLibraryへshared_ptrを渡し、Actorの生存中だけLighting対象にする。
	if(auto* ctx = SceneContext::Current()) {
		if(auto* lightLibrary = ctx->GetLightLibrary()) {
			lightLibrary->AddPointLight(pointLight_);
		}
	}
}

void PointLightActor::UnregisterPointLight() {
	if(!pointLight_) return;
	if(auto* ctx = SceneContext::Current()) {
		if(auto* lightLibrary = ctx->GetLightLibrary()) {
			lightLibrary->RemovePointLight(pointLight_);
		}
	}
}

void PointLightActor::SyncLightTransform() {
	if(!pointLight_) return;

	// ActorのWorld座標へLocal Offsetを加算し、親Transformを持つ場合も正しい位置へ配置する。
	worldTransform_.Update();
	auto& lightTransform = pointLight_->GetWorldTransform();
	lightTransform.translation = worldTransform_.GetWorldPosition() + lightLocalPosition_;
	lightTransform.Update();
	pointLight_->SyncPositionFromTransform();
}

REGISTER_SCENE_OBJECT(PointLightActor)
