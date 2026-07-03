#include "DemoCameraPivot.h"

// engine
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Scene/Context/SceneContext.h>
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"
#include "UI/Panels/InspectorPanel.h"

#include <algorithm>
#include <cmath>

DemoCameraPivot::DemoCameraPivot() {
	SceneObject::SetName("DemoCameraPivot", ObjectType::GameObject);
}

void DemoCameraPivot::ApplyConfigFromJson(const nlohmann::json& j) {
	ApplyConfig(j.get<SceneObjectConfig>());

	const std::string typeKey(GetTypeName());
	if(j.contains(typeKey)) {
		ApplyCameraPivotConfig(j.at(typeKey));
	}
}

void DemoCameraPivot::ExtractConfigToJson(nlohmann::json& j) const {
	j = ExtractConfig();

	nlohmann::json derived;
	ExtractCameraPivotConfig(derived);
	if(!derived.empty()) {
		j[std::string(GetTypeName())] = std::move(derived);
	}
}

void DemoCameraPivot::ApplyConfig(const SceneObjectConfig& config) {
	id_ = config.guid;
	parentId_ = config.parentGuid;
	objectType_ = static_cast<ObjectType>(config.objectType);
	name_ = config.name.empty() ? "DemoCameraPivot" : config.name;
	worldTransform_.ApplyConfig(config.transform);
}

SceneObjectConfig DemoCameraPivot::ExtractConfig() const {
	SceneObjectConfig config;
	config.guid = id_;
	config.parentGuid = parentId_;
	config.objectType = static_cast<int>(objectType_);
	config.name = name_;
	config.transform = const_cast<WorldTransform&>(worldTransform_).ExtractConfig();
	return config;
}

void DemoCameraPivot::ApplyCameraPivotConfig(const nlohmann::json& j) {
	// TransformRefには所有元GUIDだけを復元し、WorldTransformの解決は利用時まで遅延する。
	if(j.contains("targetTransform")) {
		targetTransform_ = j.at("targetTransform").get<CalyxEngine::TransformRef>();
	} else if(j.contains("targetPlayer")) {
		// 旧データのtargetPlayerも同じ所有元GUIDなので、後方互換として読み込める。
		targetTransform_.SetGuid(j.at("targetPlayer").get<Guid>());
	}
	autoFindTarget_ = j.value("autoFindTarget", autoFindTarget_);
	pivotLocalOffset_ = j.value("pivotLocalOffset", pivotLocalOffset_);
	cameraLocalOffset_ = j.value("cameraLocalOffset", cameraLocalOffset_);
	followSharpness_ = j.value("followSharpness", followSharpness_);
	yaw_ = j.value("yaw", yaw_);
	pitch_ = j.value("pitch", pitch_);
	stickRotateSpeed_ = j.value("stickRotateSpeed", stickRotateSpeed_);
	keyRotateSpeed_ = j.value("keyRotateSpeed", keyRotateSpeed_);
	minPitch_ = j.value("minPitch", minPitch_);
	maxPitch_ = j.value("maxPitch", maxPitch_);
}

void DemoCameraPivot::ExtractCameraPivotConfig(nlohmann::json& j) const {
	// WorldTransform*は保存せず、所有元SceneObjectのGUIDだけをシーンデータへ保存する。
	j["targetTransform"] = targetTransform_;
	j["autoFindTarget"] = autoFindTarget_;
	j["pivotLocalOffset"] = pivotLocalOffset_;
	j["cameraLocalOffset"] = cameraLocalOffset_;
	j["followSharpness"] = followSharpness_;
	j["yaw"] = yaw_;
	j["pitch"] = pitch_;
	j["stickRotateSpeed"] = stickRotateSpeed_;
	j["keyRotateSpeed"] = keyRotateSpeed_;
	j["minPitch"] = minPitch_;
	j["maxPitch"] = maxPitch_;
}

void DemoCameraPivot::AlwaysUpdate(float dt) {
	UpdateRotationInput(dt);

	// 参照先は削除され得るため、Transformポインタをフレームを越えて信用せず毎フレーム解決する。
	target_ = ResolveTargetTransform();
	if(target_) {
		ApplyPivotTransform(dt);
	} else {
		worldTransform_.Update();
	}

	mainCamera_ = ResolveMainCamera();
	ApplyCameraTransform();
}

const BaseTransform* DemoCameraPivot::ResolveTargetTransform() {
	// 明示参照がある場合、利用側にはSceneObjectを公開せず読み取り専用Transformだけを返す。
	if(targetTransform_.IsAssigned()) {
		// 未設定、別シーン、または削除済みならResolveはnullptrを返す。
		// 壊れた明示参照を別オブジェクトへ勝手に置き換えないことで設定ミスを可視化する。
		return targetTransform_.Resolve();
	}

	if(auto parent = GetParent()) {
		return &parent->GetWorldTransform();
	}

	if(!autoFindTarget_) {
		return nullptr;
	}

	auto* ctx = SceneContext::Current();
	auto* lib = ctx ? ctx->GetObjectLibrary() : nullptr;
	if(!lib) {
		return nullptr;
	}

	if(auto player = lib->FindByName("DemoPlayer")) {
		return &player->GetWorldTransform();
	}

	auto players = lib->FindByClassName("DemoPlayer");
	if(!players.empty()) {
		return &players.front()->GetWorldTransform();
	}

	return nullptr;
}

void DemoCameraPivot::RemapSceneObjectReferences(const std::unordered_map<Guid, Guid>& guidMap) {
	// 基底クラスが管理するSerializableObject内の参照を先に更新する。
	SceneObject::RemapSceneObjectReferences(guidMap);
	// このクラスが直接保持する参照も、Prefab複製先のGUIDへ付け替える。
	// Transform自体にはGUIDがないため、所有元SceneObjectのGUIDを複製先へ付け替える。
	targetTransform_.Remap(guidMap);
}

void DemoCameraPivot::UpdateRotationInput(float dt) {
	const CalyxEngine::Vector2 stickInput = CalyxFoundation::Input::GetRightStick();
	CalyxEngine::Vector2 keyInput{0.0f, 0.0f};

	if(CalyxFoundation::Input::PushKey(DIK_RIGHT)) {
		keyInput.x += 1.0f;
	}
	if(CalyxFoundation::Input::PushKey(DIK_LEFT)) {
		keyInput.x -= 1.0f;
	}
	if(CalyxFoundation::Input::PushKey(DIK_UP)) {
		keyInput.y += 1.0f;
	}
	if(CalyxFoundation::Input::PushKey(DIK_DOWN)) {
		keyInput.y -= 1.0f;
	}

	yaw_ += (stickInput.x * stickRotateSpeed_ + keyInput.x * keyRotateSpeed_) * dt;
	pitch_ = std::clamp(pitch_ - (stickInput.y * stickRotateSpeed_ + keyInput.y * keyRotateSpeed_) * dt, minPitch_, maxPitch_);
}

std::shared_ptr<SceneObject> DemoCameraPivot::ResolveMainCamera() {
	auto camera = std::dynamic_pointer_cast<SceneObject>(CameraManager::GetMain3dShared());
	if(camera && camera->GetParent().get() == this) {
		camera->SetParent(nullptr);
	}
	return camera;
}

void DemoCameraPivot::ApplyPivotTransform(float dt) {
	if(GetParent()) {
		worldTransform_.translation = pivotLocalOffset_;
		worldTransform_.rotation = CalyxEngine::Quaternion::MakeIdentity();
		worldTransform_.eulerRotation = {0.0f, 0.0f, 0.0f};
		worldTransform_.rotationSource = RotationSource::Quaternion;
		worldTransform_.Update();
		return;
	}

	if(!target_) {
		worldTransform_.Update();
		return;
	}

	const CalyxEngine::Vector3 desired = target_->GetWorldPosition() + pivotLocalOffset_;
	const float alpha = followSharpness_ <= 0.0f
		? 1.0f
		: 1.0f - std::exp(-followSharpness_ * (std::max)(dt, 0.0f));
	worldTransform_.translation = CalyxEngine::Vector3::Lerp(worldTransform_.translation, desired, std::clamp(alpha, 0.0f, 1.0f));
	worldTransform_.Update();
}

void DemoCameraPivot::ApplyCameraTransform() {
	auto camera = mainCamera_.lock();
	if(!camera) {
		return;
	}

	const CalyxEngine::Quaternion cameraRotation = CalyxEngine::Quaternion::EulerToQuaternion({pitch_, yaw_, 0.0f});
	const CalyxEngine::Vector3 pivotPosition = worldTransform_.GetWorldPosition();
	const CalyxEngine::Vector3 cameraPosition =
		pivotPosition + CalyxEngine::Quaternion::RotateVector(cameraLocalOffset_, cameraRotation);

	auto& cameraTransform = camera->GetWorldTransform();
	cameraTransform.parent = nullptr;
	cameraTransform.translation = cameraPosition;
	cameraTransform.eulerRotation = {pitch_, yaw_, 0.0f};
	cameraTransform.rotationSource = RotationSource::Euler;
	cameraTransform.Update();
}

void DemoCameraPivot::ShowGui() {
	// --- トランスフォーム ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		worldTransform_.ShowImGui("world");
		// Hierarchyから任意のSceneObjectを割り当てるが、利用側へ公開されるのはTransformだけである。
		GuiCmd::SceneObjectReferenceField("Target Transform", targetTransform_);
		GuiCmd::CheckBox("Auto Find Target", autoFindTarget_);
		GuiCmd::DragFloat3("Pivot Local Offset", pivotLocalOffset_, 0.01f, -100.0f, 100.0f);
		GuiCmd::DragFloat3("Camera Local Offset", cameraLocalOffset_, 0.01f, -100.0f, 100.0f);
		GuiCmd::DragFloat("Yaw", yaw_, 0.01f, -6.283185f, 6.283185f);
		GuiCmd::DragFloat("Pitch", pitch_, 0.01f, minPitch_, maxPitch_);
		GuiCmd::DragFloat("Stick Rotate Speed", stickRotateSpeed_, 0.01f, 0.0f, 20.0f);
		GuiCmd::DragFloat("Key Rotate Speed", keyRotateSpeed_, 0.01f, 0.0f, 20.0f);
		GuiCmd::DragFloat("Follow Sharpness", followSharpness_, 0.1f, 0.0f, 100.0f);
		GuiCmd::EndSection();
	}
}
