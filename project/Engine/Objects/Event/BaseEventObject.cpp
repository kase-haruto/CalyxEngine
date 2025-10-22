#include "BaseEventObject.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Renderer/Primitive/BoxDrawer.h>

// external
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor/dtor
/////////////////////////////////////////////////////////////////////////////////////////
BaseEventObject::BaseEventObject() {
	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider"); //< コライダー名前設定
	box->Initialize(Vector3(1.0f));          //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);
}

BaseEventObject::BaseEventObject(const std::string& name) {
	SceneObject::SetName(name,ObjectType::Event);

	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(name + "BoxCollider"); //< コライダー名前設定
	box->Initialize(Vector3(1.0f));          //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);
}


BaseEventObject::~BaseEventObject() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::AlwaysUpdate([[maybe_unused]]float dt) {

	worldTransform_.Update();

	Vector3    worldPos = worldTransform_.GetWorldPosition();
	Quaternion rot      = worldTransform_.rotation;

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			collider_->Update(worldPos,rot);
			collider_->Draw();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		debugGui
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::ShowGui() {
	worldTransform_.ShowImGui();

	DerivativeGui();
}

void BaseEventObject::DerivativeGui() {
	ImGui::SeparatorText("derivative");
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の適用
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::ApplyConfig() {
	const EventConfig& cfg = config_.GetConfig();

	// collider
	if(collider_) { collider_->ApplyConfig(cfg.colliderConfig); }

	// transform / id / parent / name
	worldTransform_.ApplyConfig(cfg.transform);
	id_       = cfg.guid;
	parentId_ = cfg.parentGuid;
	name_     = cfg.name;
}

void BaseEventObject::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();

	//派生クラスの適用
	const std::string     typeKey(GetTypeName()); // クラス名
	const nlohmann::json* derived = j.contains(typeKey) ? &j.at(typeKey) : nullptr;
	ApplyDerivedConfigFromJson(j,derived);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の掃き出し
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::ExtractConfig() {
	EventConfig& cfg = config_.GetConfig();
	if(collider_) { cfg.colliderConfig = collider_->ExtractConfig(); }

	cfg.transform  = worldTransform_.ExtractConfig();
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name       = name_;
	cfg.guid       = id_;
	cfg.parentGuid = parentId_;
}

void BaseEventObject::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<BaseEventObject*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);

	// 派生部分
	const std::string typeKey(GetTypeName());
	nlohmann::json    derived;
	ExtractDerivedConfigToJson(j,derived);
	if(!derived.is_null() && !derived.empty()) { j[typeKey] = std::move(derived); }

	// シーン側で利用できるように
	if(!GetConfigPath().empty()) { j["configPath"] = GetConfigPath(); }
}