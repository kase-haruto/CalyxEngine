#include "BaseGameObject.h"

#include "Engine/Application/UI/Panels/InspectorPanel.h"

#include <Engine/Application/UI/Panels/AssetPanel.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/objects/Collider/BoxCollider.h>
#include <Engine/objects/Collider/CapsuleCollider.h>
#include <Engine/objects/Collider/SphereCollider.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>

#include "externals/imgui/imgui.h"
#include "externals/nlohmann/json.hpp"
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

namespace {
	int NormalizeColliderKind(int kind) {
		switch(kind) {
		case 0:
			return 0;
		case 1:
			return 1;
		case 2:
			return 2;
		case 3:
			return 3;
		default:
			return 2;
		}
	}
}

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
			model_			 = std::make_unique<CalyxEngine::AnimationModel>(modelName);
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
	config_.GetConfig().colliderKind = 0;
	InitializeCollider(ColliderKind::None);
}

BaseGameObject::BaseGameObject() {
	objectModelType_ = ObjectModelType::ModelType_Unknown;	// まだ未定
	SetName("GameObject");								// 仮の名前
	worldTransform_.Update();
	SetName("GameObject");								// 仮の名前

	config_.SetOnApplied([this](const BaseGameObjectConfig&) { this->ApplyConfig(); });
	config_.GetConfig().colliderKind = 0;
	InitializeCollider(ColliderKind::None);
}

BaseGameObject::~BaseGameObject() {
	for(auto& binding : boneParentBindings_) {
		if(binding.target && binding.target->parent == binding.parentTransform.get()) {
			binding.target->parent = nullptr;
		}
	}
}

void BaseGameObject::AlwaysUpdate(float dt) {
	textureAnimator_.Update(dt);
	if(objectModelType_ != ObjectModelType::ModelType_Unknown && model_) {
		model_->Update(dt);
	}

	worldTransform_.Update();
	UpdateBoneParents();

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			CalyxEngine::Vector3	  worldPos = GetCenterPos();
			CalyxEngine::Quaternion worldRot = worldTransform_.rotation;
			collider_->Update(worldPos, worldRot);
		}
	}
	if(model_) {
		model_->SetIsDrawEnable(IsDrawEnable());
	}
}

void BaseGameObject::DrawCollider() {
	if(collider_ && collider_->IsCollisionEnubled()) {
		collider_->Draw();
	}
}

//===================================================================*/
//						引数から種類をもらって初期化
//===================================================================*/
void BaseGameObject::InitializeCollider(ColliderKind kind) {
	if(kind == currentColliderKind_ && collider_) return; // 差分がなければ早期リターン

	if(kind == ColliderKind::None) {
		collider_.reset();
		currentColliderKind_ = kind;
		config_.GetConfig().colliderKind = static_cast<int>(kind);
		return;
	}

	switch(kind) {
	// box形状のコライダーを生成
	case ColliderKind::Box: {
		auto box = std::make_unique<BoxCollider>(true);
		box->SetName(GetName() + "_BoxCollider");
		box->Initialize(CalyxEngine::Vector3(1.0f, 1.0f, 1.0f)); // 適当な初期サイズ
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
	// カプセル形状のコライダーの生成
	case ColliderKind::Capsule: {
		auto capsule = std::make_unique<CapsuleCollider>(true);
		capsule->SetName(GetName() + "_CapsuleCollider");
		capsule->Initialize(0.5f, 2.0f); // 適当な初期半径と全体高さ
		collider_ = std::move(capsule);
		break;
	}
	}

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
	collider_->SetOwner(this);

	currentColliderKind_ = kind;
	config_.GetConfig().colliderKind = static_cast<int>(kind);
}

//===================================================================*/
//                    imgui/ui
//===================================================================*/
void BaseGameObject::ShowGui() {
	ImGui::Spacing();
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();

	// --- トランスフォーム ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		worldTransform_.ShowImGui("world");
		GuiCmd::EndSection();
	}

	// --- マテリアル・モデル ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Material)) {
		if(ImGui::TreeNodeEx("Model Asset (Drag & Drop from Assets)", ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen)) {
			Guid droppedGuid = Guid::Empty();
			if(CalyxEngine::AssetPanel::DrawAssetDropTarget(AssetType::Model, &droppedGuid)) {
				if(auto* db = AssetDatabase::GetInstance()) {
					if(const AssetRecord* record = db->Get(droppedGuid); record && record->type == AssetType::Model) {
						const std::string before = config_.GetConfig().modelConfig.modelName;
						const std::string after = record->sourcePath.filename().string();
						if(before != after) {
							auto apply = [this](const std::string& modelName) {
								SetModelFileNameForEditor(modelName);
							};
							CommandManager::GetInstance()->Execute(
								std::make_unique<ValueEditCommand<std::string>>("Apply Model Asset", before, after, apply));
						}
					}
				}
			}

			ImGui::TextDisabled("Current: %s", config_.GetConfig().modelConfig.modelName.c_str());
			ImGui::TreePop();
		}

		GuiCmd::EndSection();
	}

	if(model_) {
		model_->ShowImGui(config_.GetConfig().modelConfig);
	}

	// --- コライダー ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Collider)) {
		auto applyColliderKind = [this](ColliderKind kind) { InitializeCollider(kind); };

		if(!collider_) {
			ImGui::TextDisabled("No collider");
			if(ImGui::Button("Add Collider")) {
				ImGui::OpenPopup("AddColliderPopup");
			}
			if(ImGui::BeginPopup("AddColliderPopup")) {
				if(ImGui::MenuItem("Box Collider")) {
					CommandManager::GetInstance()->Execute(
						std::make_unique<ValueEditCommand<ColliderKind>>(
							"Add Box Collider",
							ColliderKind::None,
							ColliderKind::Box,
							applyColliderKind));
				}
				if(ImGui::MenuItem("Sphere Collider")) {
					CommandManager::GetInstance()->Execute(
						std::make_unique<ValueEditCommand<ColliderKind>>(
							"Add Sphere Collider",
							ColliderKind::None,
							ColliderKind::Sphere,
							applyColliderKind));
				}
				if(ImGui::MenuItem("Capsule Collider")) {
					CommandManager::GetInstance()->Execute(
						std::make_unique<ValueEditCommand<ColliderKind>>(
							"Add Capsule Collider",
							ColliderKind::None,
							ColliderKind::Capsule,
							applyColliderKind));
				}
				ImGui::EndPopup();
			}
		} else {
			static const char* kColliderKindItems[] = {"Box", "Sphere", "Capsule"};
			int kind = 0;
			if(currentColliderKind_ == ColliderKind::Sphere) {
				kind = 1;
			} else if(currentColliderKind_ == ColliderKind::Capsule) {
				kind = 2;
			}
			if(ImGui::Combo("Collider Type", &kind, kColliderKindItems, IM_ARRAYSIZE(kColliderKindItems))) {
				const ColliderKind before = currentColliderKind_;
				const ColliderKind after =
					(kind == 0) ? ColliderKind::Box :
					(kind == 1) ? ColliderKind::Sphere :
								  ColliderKind::Capsule;
				if(before != after) {
					CommandManager::GetInstance()->Execute(
						std::make_unique<ValueEditCommand<ColliderKind>>(
							"Change Collider Type",
							before,
							after,
							applyColliderKind));
				}
			}

			if(ImGui::Button("Remove Collider")) {
				CommandManager::GetInstance()->Execute(
					std::make_unique<ValueEditCommand<ColliderKind>>(
						"Remove Collider",
						currentColliderKind_,
						ColliderKind::None,
						applyColliderKind));
			}

			if(collider_) {
				collider_->ShowGui();
				physicsBody_.ShowGui();
			} else {
				ImGui::TextDisabled("No collider");
			}
		}
		GuiCmd::EndSection();
	}

	// --- 描画設定 ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::Object)) {
		if(ImGui::TreeNodeEx("Visual Offset", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			GuiCmd::DragFloat3("Offset", visualOffset_, 0.01f, -1000.0f, 1000.0f);
			ImGui::TreePop();
		}
		if(ImGui::TreeNodeEx("Draw Config", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			GuiCmd::CheckBox("Camera Dither", drawConfig_.cameraDitherEnabled);
			GuiCmd::CheckBox("Cast Shadow", drawConfig_.castShadow);
			GuiCmd::CheckBox("Enable Outline", drawConfig_.outline.enabled);
			GuiCmd::DragFloat("Outline Thickness", drawConfig_.outline.thickness, 0.001f, 0.0f, 1.0f);
			ImGui::ColorEdit4("Outline Color", &drawConfig_.outline.color.x);
			ImGui::TreePop();
		}
		if(ImGui::TreeNodeEx("Billboard Mode", ImGuiTreeNodeFlags_SpanAvailWidth)) {
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
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::ParameterData)) {
		if(auto* animationModel = AnimationModel()) {
			animationModel->ShowImGuiInterface();
		}
		HeaderGui();
		GuiCmd::EndSection();
	}

	// --- 派生クラス用パラメータ ---
	if(GuiCmd::BeginSection(CalyxEngine::ParamFilterSection::ParameterData)) {
		DerivativeGui();
		GuiCmd::EndSection();
	}
}

void BaseGameObject::HeaderGui() {}

void BaseGameObject::DerivativeGui() { ImGui::SeparatorText("derivative"); }

void BaseGameObject::ApplyConfig() {
	const BaseGameObjectConfig& cfg = config_.GetConfig();

	// シーン保存時のモデル名からRuntime描画モデルを復元し、共有Asset設定を適用する。
	SetModelFromFileName(cfg.modelConfig.modelName);

	// モデル生成に失敗した旧シーンでも、残りのオブジェクト設定は復元を継続する。
	if(model_)
		model_->ApplyConfig(cfg.modelConfig);

	// 保存された形状種別に合わせて所有コライダーを再構築してから形状設定を反映する。
	InitializeCollider(static_cast<ColliderKind>(NormalizeColliderKind(cfg.colliderKind)));
	if(collider_)
		collider_->ApplyConfig(cfg.colliderConfig);

	// Colliderとは独立して保存された物理応答パラメータを復元する。
	physicsBody_.ApplyConfig(cfg.physicsBodyConfig);

	// 保存済みの配置を再現するため、ConfigからRuntime Transformを復元する。
	worldTransform_.ApplyConfig(cfg.transform);

	// モデル原点と見た目の中心が異なるAsset向けの描画補正を復元する。
	visualOffset_ = cfg.visualOffset;

	// Rendererへ渡すカメラディザーとOutline設定を復元する。
	drawConfig_.cameraDitherEnabled = cfg.cameraDitherEnabled;
	drawConfig_.outline.enabled	 = cfg.outlineEnabled;
	drawConfig_.outline.thickness = cfg.outlineThickness;
	drawConfig_.outline.color	 = cfg.outlineColor;
	// Scene参照と親子関係を再接続できるよう永続GUIDを最後に復元する。
	id_		  = cfg.guid;
	parentId_ = cfg.parentGuid;
	name_	  = cfg.name;
}

void BaseGameObject::ExtractConfig() {
	BaseGameObjectConfig& cfg = config_.GetConfig();

	// EditorまたはRuntimeで変更された所有コンポーネントの状態をシーン保存形式へ戻す。
	if(model_)
		cfg.modelConfig = model_->ExtractConfig();

	// Collider未設定時は既存Configを保持し、None形状として種別だけを保存する。
	if(collider_)
		cfg.colliderConfig = collider_->ExtractConfig();

	// 物理応答設定をCollider形状とは独立したConfigへ抽出する。
	cfg.physicsBodyConfig = physicsBody_.ExtractConfig();
	cfg.colliderKind = static_cast<int>(currentColliderKind_);

	// 公開Transform APIの名前は維持しつつ、安定したConfig/JSONキーへ変換して互換性を保つ。
	cfg.transform  = worldTransform_.ExtractConfig();

	// Editor固有の描画補正もRuntimeで同じ外観を再現できるよう永続化する。
	cfg.visualOffset = visualOffset_;
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name	   = name_;
	cfg.guid	   = id_;
	cfg.parentGuid = parentId_;
	// Renderer設定をScene保存用のPOD値へ展開する。
	cfg.cameraDitherEnabled = drawConfig_.cameraDitherEnabled;
	cfg.outlineEnabled	 = drawConfig_.outline.enabled;
	cfg.outlineThickness = drawConfig_.outline.thickness;
	cfg.outlineColor	 = drawConfig_.outline.color;
}

void BaseGameObject::ApplyConfigFromJson(const nlohmann::json& j) {
	// 共通設定を先に復元し、派生設定が利用するモデルやコライダーを準備する。
	config_.ApplyConfigFromJson(j);
	ApplyConfig();

	// 旧データに派生型キーがない場合も、共通JSONを渡して派生側の互換読込を許可する。
	const std::string	  typeKey(GetTypeName());
	const nlohmann::json* derived = j.contains(typeKey) ? &j.at(typeKey) : nullptr;
	ApplyDerivedConfigFromJson(j, derived);
}

void BaseGameObject::ExtractConfigToJson(nlohmann::json& j) const {
	// 現在値をConfigへ同期してから、SceneSerializerが扱う共通JSONへ書き出す。
	const_cast<BaseGameObject*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);

	// 派生固有設定は型名キーへ隔離し、共通設定とのキー衝突を避ける。
	const std::string typeKey(GetTypeName());
	nlohmann::json	  derived;
	ExtractDerivedConfigToJson(j, derived);
	if(!derived.is_null() && !derived.empty()) {
		j[typeKey] = std::move(derived);
	}

	// 外部Configを参照する既存シーンとの互換性のため、設定元パスも保存する。
	if(!GetConfigPath().empty()) {
		j["configPath"] = GetConfigPath();
	}
}

//===================================================================*/
//                   getter/setter
//===================================================================*/

void BaseGameObject::SetName(const std::string& name) { SceneObject::SetName(name, ObjectType::GameObject); }

void BaseGameObject::SetTranslate(const CalyxEngine::Vector3& pos) {
	if(model_) {
		worldTransform_.translation = pos;
	}
}
void BaseGameObject::SetRotate(const CalyxEngine::Quaternion& rot) {
	if(model_) {
		worldTransform_.rotation = rot;
	}
}
void BaseGameObject::SetRotate(const CalyxEngine::Vector3& euler) {
	if(model_) {
		worldTransform_.eulerRotation = euler;
	}
}

void BaseGameObject::SetScale(const CalyxEngine::Vector3& scale) {
	if(model_) {
		worldTransform_.scale = scale;
	}
}

void BaseGameObject::SetDrawEnable(bool isDrawEnable) {
	SceneObject::SetDrawEnable(isDrawEnable);
	if(model_) {
		model_->SetIsDrawEnable(isDrawEnable);
	}
}

const CalyxEngine::Vector3 BaseGameObject::GetCenterPos() const {
	// BaseGameObjectの衝突中心は、モデルが持つ実際のワールドAABB中心を基準にする。
	// Transform原点は「配置基準点」であり、Planeやアセットによっては見た目の中心と一致しない。
	// ここで固定値のオフセットを入れると形状ごとに破綻するため、描画モデルの境界から中心を求める。
	if(model_ && objectModelType_ != ObjectModelType::ModelType_Unknown) {
		const AABB worldAabb = GetWorldAABB();
		return (worldAabb.min_ + worldAabb.max_) * 0.5f;
	}

	// モデルを持たないオブジェクトは、配置基準点を中心として扱う。
	return worldTransform_.GetWorldPosition();
}

void BaseGameObject::SetColor(const CalyxEngine::Vector4& color) {
	if(model_) {
		model_->SetColor(color);
	}
}

void BaseGameObject::SetCollider(std::unique_ptr<Collider> collider) {
	collider_ = std::move(collider);
	if(!collider_) {
		currentColliderKind_ = ColliderKind::None;
		config_.GetConfig().colliderKind = static_cast<int>(currentColliderKind_);
		return;
	}

	if(dynamic_cast<BoxCollider*>(collider_.get())) {
		currentColliderKind_ = ColliderKind::Box;
	} else if(dynamic_cast<SphereCollider*>(collider_.get())) {
		currentColliderKind_ = ColliderKind::Sphere;
	} else if(dynamic_cast<CapsuleCollider*>(collider_.get())) {
		currentColliderKind_ = ColliderKind::Capsule;
	} else {
		currentColliderKind_ = ColliderKind::None;
	}

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
	collider_->SetOwner(this);
	config_.GetConfig().colliderKind = static_cast<int>(currentColliderKind_);
}

Collider* BaseGameObject::GetCollider() { return collider_.get(); }

void BaseGameObject::SetTexture(const std::string& texName) { model_->SetTex(texName); }

bool BaseGameObject::SetModelByGuid(const Guid& guid) {
	if(!guid.isValid()) return false;

	auto* db = AssetDatabase::GetInstance();
	if(!db) return false;

	const AssetRecord* record = db->Get(guid);
	if(!record || record->type != AssetType::Model) return false;

	return SetModelFileNameForEditor(record->sourcePath.filename().string());
}

bool BaseGameObject::SetModelFileNameForEditor(const std::string& modelName) {
	BaseModelConfig& modelConfig = config_.GetConfig().modelConfig;
	const std::string previousModelName = modelConfig.modelName;
	modelConfig.modelName = modelName;

	if(!SetModelFromFileName(modelConfig.modelName)) {
		CalyxEngine::EngineLogger::GetInstance().Add(
			CalyxEngine::LogLevel::Error,
			CalyxEngine::LogCategory::Asset,
			"Model assignment failed for object '" + GetName() + "': " + modelName,
			"BaseGameObject");
		return false;
	}
	if(model_) {
		model_->ApplyConfig(modelConfig);
	}
	if(previousModelName != modelName) {
		CalyxEngine::EngineLogger::GetInstance().Add(
			CalyxEngine::LogLevel::Info,
			CalyxEngine::LogCategory::Asset,
			"Model changed for object '" + GetName() + "': " + previousModelName + " -> " + modelName,
			"BaseGameObject");
	}

	return true;
}

bool BaseGameObject::SetModelFromFileName(const std::string& modelName) {
	if(modelName.empty()) return false;

	auto dot = modelName.find_last_of('.');
	if(dot == std::string::npos) return false;

	const std::string extension = modelName.substr(dot);

	if(extension == ".gltf") {
		objectModelType_ = ObjectModelType::ModelType_Animation;
		model_			 = std::make_unique<CalyxEngine::AnimationModel>(modelName);
		return true;
	}

	objectModelType_ = ObjectModelType::ModelType_Static;
	model_			 = std::make_unique<Model>(modelName);
	return true;
}

bool BaseGameObject::RegisterAnimationClip(
	int16_t animId,
	const std::string& animName,
	const std::optional<std::string>& fileName) {
	auto* animModel = AnimationModel();
	if(!animModel) return false;

	animModel->RegisterAnimation(animId, animName, fileName);
	return true;
}

void BaseGameObject::PlayRegisteredAnimation(int16_t animId, float blendDuration) {
	auto* animModel = AnimationModel();
	if(!animModel) return;

	animModel->Play(animId, blendDuration);
}

Model* BaseGameObject::GetStaticModel() {
	return (objectModelType_ == ObjectModelType::ModelType_Static)
			   ? static_cast<Model*>(model_.get())
			   : nullptr;
}

CalyxEngine::AnimationModel* BaseGameObject::AnimationModel() {
	return (objectModelType_ == ObjectModelType::ModelType_Animation)
			   ? static_cast<CalyxEngine::AnimationModel*>(model_.get())
			   : nullptr;
}

const CalyxEngine::AnimationModel* BaseGameObject::AnimationModel() const {
	return (objectModelType_ == ObjectModelType::ModelType_Animation)
			   ? static_cast<CalyxEngine::AnimationModel*>(model_.get())
			   : nullptr;
}

static inline AABB TransformAabb(const AABB& local, const CalyxEngine::Matrix4x4& W) {
	const CalyxEngine::Vector3 lc	 = (local.min_ + local.max_) * 0.5f;
	const CalyxEngine::Vector3 le0 = (local.max_ - local.min_) * 0.5f;

	const CalyxEngine::Vector3 wc = (W * CalyxEngine::Vector4(lc, 1.0f)).xyz();

	const float m00 = std::fabs(W.m[0][0]), m01 = std::fabs(W.m[0][1]), m02 = std::fabs(W.m[0][2]);
	const float m10 = std::fabs(W.m[1][0]), m11 = std::fabs(W.m[1][1]), m12 = std::fabs(W.m[1][2]);
	const float m20 = std::fabs(W.m[2][0]), m21 = std::fabs(W.m[2][1]), m22 = std::fabs(W.m[2][2]);

	const CalyxEngine::Vector3 we = {
		m00 * le0.x + m01 * le0.y + m02 * le0.z,
		m10 * le0.x + m11 * le0.y + m12 * le0.z,
		m20 * le0.x + m21 * le0.y + m22 * le0.z};
	return AABB(wc - we, wc + we);
}

AABB BaseGameObject::GetWorldAABB() const {
	const CalyxEngine::Matrix4x4& W = worldTransform_.matrix.world;

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

WorldTransform BaseGameObject::GetRenderWorldTransform() const {
	WorldTransform renderTransform = worldTransform_;
	if(visualOffset_.LengthSquared() <= 1.0e-10f) {
		return renderTransform;
	}

	renderTransform.translation += visualOffset_;
	renderTransform.Update();
	return renderTransform;
}

void BaseGameObject::BoneParentTransform::SetWorldMatrix(const CalyxEngine::Matrix4x4& world) {
	matrix.world				 = world;
	matrix.WorldInverseTranspose = CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(world));
	++revision_;
}

void BaseGameObject::SetBoneParent(WorldTransform& target, const std::string& boneName, bool inheritScale) {
	ClearBoneParent(target);

	BoneParentBinding binding;
	binding.target			= &target;
	binding.boneName		= boneName;
	binding.inheritScale	= inheritScale;
	binding.parentTransform = std::make_unique<BoneParentTransform>();

	target.parent		 = binding.parentTransform.get();
	target.inheritScale = inheritScale;

	boneParentBindings_.push_back(std::move(binding));
	worldTransform_.Update();
	UpdateBoneParents();
}

void BaseGameObject::ClearBoneParent(WorldTransform& target) {
	boneParentBindings_.erase(
		std::remove_if(
			boneParentBindings_.begin(),
			boneParentBindings_.end(),
			[&target](const BoneParentBinding& binding) {
				if(binding.target == &target) {
					target.parent = nullptr;
					return true;
				}
				return false;
			}),
		boneParentBindings_.end());
}

bool BaseGameObject::HasBoneParentTarget(const WorldTransform* target) const {
	if(!target) return false;
	return std::any_of(
		boneParentBindings_.begin(),
		boneParentBindings_.end(),
		[target](const BoneParentBinding& binding) {
			return binding.target == target;
		});
}

std::optional<std::string> BaseGameObject::GetBoneParentNameForTarget(const WorldTransform* target) const {
	if(!target) return std::nullopt;
	for(const auto& binding : boneParentBindings_) {
		if(binding.target == target) {
			return binding.boneName;
		}
	}
	return std::nullopt;
}

std::vector<std::string> BaseGameObject::GetBoneNamesForEditor() const {
	auto* animModel = AnimationModel();
	if(!animModel) return {};
	return animModel->GetJointNames();
}

bool BaseGameObject::IsSkeletonDrawEnabledForEditor() const {
	auto* animModel = AnimationModel();
	return animModel && animModel->IsDrawSkeletonEnabled();
}

void BaseGameObject::SetSkeletonDrawEnabledForEditor(bool enabled) {
	auto* animModel = AnimationModel();
	if(!animModel) return;
	animModel->SetDrawSkeletonEnabled(enabled);
}

bool BaseGameObject::SelectBoneForEditor(const std::string& boneName) {
	auto* animModel = AnimationModel();
	if(!animModel) return false;
	return animModel->SetSelectedJointByName(boneName);
}

std::vector<BaseGameObject::BoneParentBindingInfo> BaseGameObject::GetBoneParentBindings() const {
	std::vector<BoneParentBindingInfo> result;
	result.reserve(boneParentBindings_.size());
	for(const auto& binding : boneParentBindings_) {
		result.push_back(BoneParentBindingInfo{
			binding.target,
			binding.boneName,
			binding.inheritScale});
	}
	return result;
}

void BaseGameObject::UpdateBoneParents() {
	auto* animModel = AnimationModel();
	if(!animModel) return;

	for(auto& binding : boneParentBindings_) {
		if(!binding.target || !binding.parentTransform) continue;

		auto jointMatrix = animModel->GetJointMatrix(binding.boneName);
		if(!jointMatrix.has_value()) continue;

		binding.parentTransform->SetWorldMatrix(jointMatrix.value() * worldTransform_.matrix.world);
		binding.target->parent		 = binding.parentTransform.get();
		binding.target->inheritScale = binding.inheritScale;
	}
}

bool BaseGameObject::Save() const {
	const std::string& path = GetConfigPath(); // SceneObject の保持値を使う
	if(path.empty()) return false;
	nlohmann::json j;
	ExtractConfigToJson(j);
	return CalyxEngine::JsonUtils::Save(path, j);
}

bool BaseGameObject::Load() {
	const std::string& path = GetConfigPath();
	if(path.empty()) return false;
	nlohmann::json j;
	if(!CalyxEngine::JsonUtils::Load(path, j)) return false;
	ApplyConfigFromJson(j);
	return true;
}

