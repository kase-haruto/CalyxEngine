#include "BaseEventObject.h"
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>

// external
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

REGISTER_SCENE_OBJECT(BaseEventObject);

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor/dtor
/////////////////////////////////////////////////////////////////////////////////////////
BaseEventObject::BaseEventObject() {
	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider");   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });

	model_ = std::make_unique<Model>("debugCube.obj");
	model_->SetBlendMode(BlendMode::ALPHA);
	model_->SetColor(CalyxMath::Vector4(0.0f, 1.0f, 0.0f, 0.5f));

	isCastShadow_ = false; // 影を落とさない

	baseConfig_.SetOnApplied([this](const EventConfig&) {
		this->ApplyConfig();
	});
	baseConfig_.SetOnExtracted([this](const EventConfig&) {
		this->ExtractConfig();
	});
}

BaseEventObject::BaseEventObject(const std::string& name) {
	SceneObject::SetName(name, ObjectType::Event);

	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(name + "BoxCollider");		   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });

	model_ = std::make_unique<Model>("debugCube.obj");
	model_->SetBlendMode(BlendMode::ALPHA);
	model_->SetColor(CalyxMath::Vector4(0.0f, 1.0f, 0.0f, 0.5f));

	isCastShadow_ = false; // 影を落とさない

	baseConfig_.SetOnApplied([this](const EventConfig&) {
		this->ApplyConfig();
	});
	baseConfig_.SetOnExtracted([this](const EventConfig&) {
		this->ExtractConfig();
	});
}

BaseEventObject::~BaseEventObject() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::Initialize() {
	// 個別の調節パラメータ適用
	const std::string configRoot = "Event/";
	baseConfig_.LoadConfig(configRoot + GetName());

	// debug/developのみ描画
#if defined(_DEBUG) || defined(DEVELOP)
	isDrawEnable_ = true;
#else
	isDrawEnable_ = false;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::AlwaysUpdate([[maybe_unused]] float dt) {

	worldTransform_.Update();

	if(model_) {
		model_->Update(dt);
		model_->SetIsDrawEnable(isDrawEnable_);
	}

	CalyxMath::Vector3	  worldPos = worldTransform_.GetWorldPosition();
	CalyxMath::Quaternion rot	   = worldTransform_.rotation;

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			collider_->Update(worldPos, rot);
			auto* box = dynamic_cast<BoxCollider*>(collider_.get());
			if(box) box->SetSize(worldTransform_.scale);
			// collider_->Draw();	// 線の描画は止めてモデルで代替
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		debugGui
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::ShowGui() {
	ConfigGUi();
	worldTransform_.ShowImGui();
	DerivativeGui();
}

void BaseEventObject::DerivativeGui() {
	ImGui::SeparatorText("objectParam");
	model_->ShowImGuiInterface();
}

void BaseEventObject::ConfigGUi() {
	baseConfig_.ShowGui("Event/" + GetName());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の適用
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::ApplyConfig() {
	const EventConfig& cfg = baseConfig_.GetConfig();

	// collider
	if(collider_) {
		collider_->ApplyConfig(cfg.colliderConfig);
	}

	// transform / id / parent / name
	worldTransform_.ApplyConfig(cfg.transform);
	id_		  = cfg.guid;
	parentId_ = cfg.parentGuid;

	if(!cfg.name.empty()) {
		// 空文字以外の場合代入
		name_ = cfg.name;
	}
}

void BaseEventObject::ApplyConfigFromJson(const nlohmann::json& j) {
	baseConfig_.ApplyConfigFromJson(j);
	ApplyConfig();

	// 派生クラスの適用
	const std::string	  typeKey(GetTypeName()); // クラス名
	const nlohmann::json* derived = j.contains(typeKey) ? &j.at(typeKey) : nullptr;
	ApplyDerivedConfigFromJson(j, derived);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の掃き出し
/////////////////////////////////////////////////////////////////////////////////////////
void BaseEventObject::ExtractConfig() {
	EventConfig& cfg = baseConfig_.GetConfig();
	if(collider_) {
		cfg.colliderConfig = collider_->ExtractConfig();
	}

	cfg.transform  = worldTransform_.ExtractConfig();
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name	   = name_;
	cfg.guid	   = id_;
	cfg.parentGuid = parentId_;
}

void BaseEventObject::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<BaseEventObject*>(this)->ExtractConfig();
	baseConfig_.ExtractConfigToJson(j);

	// 派生部分
	const std::string typeKey(GetTypeName());
	nlohmann::json	  derived;
	ExtractDerivedConfigToJson(j, derived);
	if(!derived.is_null() && !derived.empty()) {
		j[typeKey] = std::move(derived);
	}

	// シーン側で利用できるように
	if(!GetConfigPath().empty()) {
		j["configPath"] = GetConfigPath();
	}
}