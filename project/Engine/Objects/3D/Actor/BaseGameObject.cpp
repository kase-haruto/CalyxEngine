#include "BaseGameObject.h"

#include "Engine/Application/UI/Panels/InspectorPanel.h"

#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/objects/Collider/BoxCollider.h>
#include <Engine/objects/Collider/SphereCollider.h>

#include "externals/imgui/imgui.h"
#include "externals/nlohmann/json.hpp"
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

BaseGameObject::BaseGameObject(const std::string&		  modelName,
							   std::optional<std::string> objectName) {
	auto dotPos = modelName.find_last_of('.');
	if(dotPos != std::string::npos) {
		std::string extension = modelName.substr(dotPos);

		// obj
		if(extension == ".obj") {
			objectModelType_ = ObjectModelType::ModelType_Static;
			model_			 = std::make_unique<Model>(modelName);
		}
		// gltf
		else if(extension == ".gltf") {
			objectModelType_ = ObjectModelType::ModelType_Animation;
			model_			 = std::make_unique<CalyxAssets::AnimationModel>(modelName);
		} else {
			objectModelType_ = ObjectModelType::ModelType_Unknown;
		}
	}

	// 名前を設定
	if(objectName.has_value()) {
		SetName(objectName.value());
	} else {
		// 名前が指定されていない場合は、デフォルトの名前を設定
		const std::string defaultName = modelName + "object";
		SetName(defaultName);
	}

	//===================================================================*/
	//			collider 設定
	//===================================================================*/
	config_.SetOnApplied([this](const BaseGameObjectConfig&) { this->ApplyConfig(); });
	InitializeCollider(ColliderKind::Sphere);
}

BaseGameObject::BaseGameObject() {
	objectModelType_ = ObjectModelType::ModelType_Unknown; // まだ未定
	SetName("GameObject");								   // 仮の名前
	worldTransform_.Update();

	config_.SetOnApplied([this](const BaseGameObjectConfig&) { this->ApplyConfig(); });
}

BaseGameObject::~BaseGameObject() {}

void BaseGameObject::AlwaysUpdate(float dt) {
	if(objectModelType_ != ObjectModelType::ModelType_Unknown && model_) {
		model_->Update(dt);
	}

	worldTransform_.Update();

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			CalyxMath::Vector3	  worldPos = GetCenterPos();
			CalyxMath::Quaternion worldRot = worldTransform_.rotation;
			collider_->Update(worldPos, worldRot);
			collider_->Draw();
		}
	}
	if(model_) {
		model_->SetIsDrawEnable(isDrawEnable_);
	}
}

//===================================================================*/
//						引数から種類をもらって初期化
//===================================================================*/
void BaseGameObject::InitializeCollider(ColliderKind kind) {
	if(kind == currentColliderKind_) return; // 差分がなければ早期リターン

	switch(kind) {
	// box形状のコライダーを生成
	case ColliderKind::Box: {
		auto box = std::make_unique<BoxCollider>(true);
		box->SetName(GetName() + "_BoxCollider");
		box->Initialize(CalyxMath::Vector3(1.0f, 1.0f, 1.0f)); // 適当な初期サイズ
		collider_ = std::move(box);
		break;
	}
	// 球体形状のコライダーの生成
	case ColliderKind::Sphere: {
		auto sphere = std::make_unique<SphereCollider>(true);
		sphere->SetName(GetName() + "_SphereCollider");
		sphere->Initialize(1.0f); // 適当な初期半径
		collider_ = std::move(sphere);
		break;
	}
	}

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });

	currentColliderKind_ = kind;
}

//===================================================================*/
//                    imgui/ui
//===================================================================*/
void BaseGameObject::ShowGui() {
	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();

	// --- トランスフォーム ---
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Object)) {
		worldTransform_.ShowImGui("world");
		GuiCmd::EndSection();
	}

	// --- マテリアル・モデル ---
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Material)) {
		model_->ShowImGui(config_.GetConfig().modelConfig);
		GuiCmd::EndSection();
	}

	// --- コライダー ---
	if(collider_) {
		if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Collider)) {
			collider_->ShowGui();
			GuiCmd::EndSection();
		}
	}

	// --- 描画設定 ---
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Object)) {
		if(ImGui::TreeNodeEx("Billboard Mode", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
			int			mode	= static_cast<int>(billboardMode_);
			const char* items[] = {"None", "Full", "AxisY"};
			if(GuiCmd::Combo("Billboard Mode", mode, items, 3)) {
				billboardMode_ = static_cast<BillboardMode>(mode);
			}
			ImGui::TreePop();
		}
		GuiCmd::EndSection();
	}

	// --- パラメータデータ ---
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		HeaderGui();
		GuiCmd::EndSection();
	}

	// --- 派生クラス用パラメータ ---
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		DerivativeGui();
		GuiCmd::EndSection();
	}
}

void BaseGameObject::HeaderGui() { config_.ShowGui(); }

void BaseGameObject::DerivativeGui() { ImGui::SeparatorText("derivative"); }

void BaseGameObject::ApplyConfig() {
	const BaseGameObjectConfig& cfg = config_.GetConfig();

	const std::string& modelPath = cfg.modelConfig.modelName;
	if(!modelPath.empty()) {
		auto dot = modelPath.find_last_of('.');
		if(dot != std::string::npos && modelPath.substr(dot) == ".gltf") {
			objectModelType_ = ObjectModelType::ModelType_Animation;
			model_			 = std::make_unique<CalyxAssets::AnimationModel>(modelPath);
		} else {
			objectModelType_ = ObjectModelType::ModelType_Static;
			model_			 = std::make_unique<Model>(modelPath);
		}
	}

	if(model_)
		model_->ApplyConfig(cfg.modelConfig);
	if(collider_)
		collider_->ApplyConfig(cfg.colliderConfig);
	worldTransform_.ApplyConfig(cfg.transform);
	id_		  = cfg.guid;
	parentId_ = cfg.parentGuid;
	name_	  = cfg.name;
}

void BaseGameObject::ExtractConfig() {
	BaseGameObjectConfig& cfg = config_.GetConfig();

	if(model_)
		cfg.modelConfig = model_->ExtractConfig();
	if(collider_)
		cfg.colliderConfig = collider_->ExtractConfig();
	cfg.transform  = worldTransform_.ExtractConfig();
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name	   = name_;
	cfg.guid	   = id_;
	cfg.parentGuid = parentId_;
}

void BaseGameObject::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();

	// 派生
	const std::string	  typeKey(GetTypeName()); // クラス名
	const nlohmann::json* derived = j.contains(typeKey) ? &j.at(typeKey) : nullptr;
	ApplyDerivedConfigFromJson(j, derived);
}

void BaseGameObject::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<BaseGameObject*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);

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

//===================================================================*/
//                   getter/setter
//===================================================================*/

void BaseGameObject::SetName(const std::string& name) { SceneObject::SetName(name, ObjectType::GameObject); }

void BaseGameObject::SetTranslate(const CalyxMath::Vector3& pos) {
	if(model_) {
		worldTransform_.translation = pos;
	}
}
void BaseGameObject::SetRotate(const CalyxMath::Quaternion& rot) {
	if(model_) {
		worldTransform_.rotation = rot;
	}
}
void BaseGameObject::SetRotate(const CalyxMath::Vector3& euler) {
	if(model_) {
		worldTransform_.eulerRotation = euler;
	}
}

void BaseGameObject::SetScale(const CalyxMath::Vector3& scale) {
	if(model_) {
		worldTransform_.scale = scale;
	}
}

void BaseGameObject::SetDrawEnable(bool isDrawEnable) {
	SceneObject::SetDrawEnable(isDrawEnable);
	model_->SetIsDrawEnable(isDrawEnable);
}

const CalyxMath::Vector3 BaseGameObject::GetCenterPos() const {
	const CalyxMath::Vector3 offset	  = {0.0f, 0.5f, 0.0f};
	CalyxMath::Vector3		 worldPos = CalyxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

void BaseGameObject::SetColor(const CalyxMath::Vector4& color) {
	if(model_) {
		model_->SetColor(color);
	}
}

void BaseGameObject::SetCollider(std::unique_ptr<Collider> collider) { collider_ = std::move(collider); }

Collider* BaseGameObject::GetCollider() { return collider_.get(); }

void BaseGameObject::SetTexture(const std::string& texName) { model_->SetTex(texName); }

Model* BaseGameObject::GetStaticModel() {
	return (objectModelType_ == ObjectModelType::ModelType_Static)
			   ? static_cast<Model*>(model_.get())
			   : nullptr;
}

CalyxAssets::AnimationModel* BaseGameObject::AnimationModel() {
	return (objectModelType_ == ObjectModelType::ModelType_Animation)
			   ? static_cast<CalyxAssets::AnimationModel*>(model_.get())
			   : nullptr;
}

const CalyxAssets::AnimationModel* BaseGameObject::AnimationModel() const {
	return (objectModelType_ == ObjectModelType::ModelType_Animation)
			   ? static_cast<CalyxAssets::AnimationModel*>(model_.get())
			   : nullptr;
}

static inline AABB TransformAabb(const AABB& local, const CalyxMath::Matrix4x4& W) {
	const CalyxMath::Vector3 lc	 = (local.min_ + local.max_) * 0.5f;
	const CalyxMath::Vector3 le0 = (local.max_ - local.min_) * 0.5f;

	const CalyxMath::Vector3 wc = (W * CalyxMath::Vector4(lc, 1.0f)).xyz();

	const float m00 = std::fabs(W.m[0][0]), m01 = std::fabs(W.m[0][1]), m02 = std::fabs(W.m[0][2]);
	const float m10 = std::fabs(W.m[1][0]), m11 = std::fabs(W.m[1][1]), m12 = std::fabs(W.m[1][2]);
	const float m20 = std::fabs(W.m[2][0]), m21 = std::fabs(W.m[2][1]), m22 = std::fabs(W.m[2][2]);

	const CalyxMath::Vector3 we = {
		m00 * le0.x + m01 * le0.y + m02 * le0.z,
		m10 * le0.x + m11 * le0.y + m12 * le0.z,
		m20 * le0.x + m21 * le0.y + m22 * le0.z};
	return AABB(wc - we, wc + we);
}

AABB BaseGameObject::GetWorldAABB() const {
	const CalyxMath::Matrix4x4& W = worldTransform_.matrix.world;

	if(objectModelType_ == ModelType_Static) {
		if(model_ && model_->GetModelData()) {
			const AABB& local = model_->GetModelData()->localAABB;
			return TransformAabb(local, W);
		}
	} else { // スキン
		const auto* animModel = AnimationModel();
		if(animModel && animModel->GetModelData()) {
			const AABB& local = animModel->GetModelData()->localAABB;
			return TransformAabb(local, W);
		}
	}

	return SceneObject::FallbackAABBFromTransform();
}

bool BaseGameObject::Save() const {
	const std::string& path = GetConfigPath(); // SceneObject の保持値を使う
	if(path.empty()) return false;
	nlohmann::json j;
	ExtractConfigToJson(j);
	return CalyxUtil::JsonUtils::Save(path, j);
}

bool BaseGameObject::Load() {
	const std::string& path = GetConfigPath();
	if(path.empty()) return false;
	nlohmann::json j;
	if(!CalyxUtil::JsonUtils::Load(path, j)) return false;
	ApplyConfigFromJson(j);
	return true;
}

REGISTER_SCENE_OBJECT(BaseGameObject)