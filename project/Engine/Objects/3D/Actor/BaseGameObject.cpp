#include "BaseGameObject.h"

#include <Engine/objects/Collider/BoxCollider.h>
#include <Engine/objects/Collider/SphereCollider.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Renderer/Primitive/PrimitiveDrawer.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#include "externals/imgui/imgui.h"

BaseGameObject::BaseGameObject(const std::string& modelName,
							   std::optional<std::string> objectName){
	auto dotPos = modelName.find_last_of('.');
	if (dotPos != std::string::npos){
		std::string extension = modelName.substr(dotPos);

		// obj
		if (extension == ".obj"){
			objectModelType_ = ObjectModelType::ModelType_Static;
			model_ = std::make_unique<Model>(modelName);
		}
		// gltf
		else if (extension == ".gltf"){
			objectModelType_ = ObjectModelType::ModelType_Animation;
			model_ = std::make_unique<AnimationModel>(modelName);
		} else{
			objectModelType_ = ObjectModelType::ModelType_Unknown;
		}

	}

	// 名前を設定
	if (objectName.has_value()){
		SetName(objectName.value());
	} else{
		// 名前が指定されていない場合は、デフォルトの名前を設定
		const std::string defaultName = modelName + "object";
		SetName(defaultName);
	}

	//===================================================================*/
	//			collider 設定
	//===================================================================*/
	SwitchCollider(ColliderKind::Box, true); // 初期化時にBoxをセット

	//// コンフィグパスの生成 preset名はdefault
	//SceneObject::SetConfigPath(ConfigPathResolver::ResolvePath(GetObjectTypeName(), GetName()));
	////コンフィグの適用
	//LoadConfig(configPath_);
}

BaseGameObject::BaseGameObject(){
	objectModelType_ = ObjectModelType::ModelType_Unknown;    // まだ未定
	SetName("GameObject");                                    // 仮の名前
	SwitchCollider(ColliderKind::Box, false);                 // とりあえず Box
	worldTransform_.Update();
}

BaseGameObject::~BaseGameObject(){}

void BaseGameObject::AlwaysUpdate(float dt){
	if (objectModelType_ == ObjectModelType::ModelType_Static){
		model_->Update(dt);
	} else if (objectModelType_ == ObjectModelType::ModelType_Static){
		animationModel_->Update(dt);
	}

	worldTransform_.Update();

	// collider の更新
	if (collider_){
		if (collider_->IsCollisionEnubled()){
			Vector3 worldPos = GetCenterPos();
			Quaternion worldRot = worldTransform_.rotation;
			collider_->Update(worldPos, worldRot);
			collider_->Draw();
		}
	}
	model_->SetIsDrawEnable(isDrawEnable_);
}

//===================================================================*/
//                    コライダー形状の変更
//===================================================================*/
void BaseGameObject::SwitchCollider(ColliderKind kind, bool isCollisionEnubled){
	if (kind == currentColliderKind_) return;

	switch (kind){
		case ColliderKind::Box:
		{
			auto box = std::make_unique<BoxCollider>(isCollisionEnubled);
			box->SetName(GetName() + "_BoxCollider");
			box->Initialize(Vector3(1.0f, 1.0f, 1.0f)); // 適当な初期サイズ
			collider_ = std::move(box);
			break;
		}
		case ColliderKind::Sphere:
		{
			auto sphere = std::make_unique<SphereCollider>(isCollisionEnubled);
			sphere->SetName(GetName() + "_SphereCollider");
			sphere->Initialize(1.0f); // 適当な初期半径
			collider_ = std::move(sphere);
			break;
		}
	}
	currentColliderKind_ = kind;
}


//===================================================================*/
//                    imgui/ui
//===================================================================*/
void BaseGameObject::ShowGui(){
	ImGui::Spacing();

	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	ImGui::Separator();

	config_.ShowGui();

	worldTransform_.ShowImGui("world");

	model_->ShowImGuiInterface();

	collider_->ShowGui();

	DerivativeGui();
}

void BaseGameObject::DerivativeGui(){
	ImGui::SeparatorText("derivative");
}

void BaseGameObject::ApplyConfig(){
	const BaseGameObjectConfig& cfg = config_.GetConfig();

	const std::string& modelPath = cfg.modelConfig.modelName;
	if (!modelPath.empty()){
		auto dot = modelPath.find_last_of('.');
		if (dot != std::string::npos && modelPath.substr(dot) == ".gltf"){
			objectModelType_ = ObjectModelType::ModelType_Animation;
			model_ = std::make_unique<AnimationModel>(modelPath);
		} else{
			objectModelType_ = ObjectModelType::ModelType_Static;
			model_ = std::make_unique<Model>(modelPath);
		}
	}

	if (model_) model_->ApplyConfig(cfg.modelConfig);
	if (collider_) collider_->ApplyConfig(cfg.colliderConfig);
	worldTransform_.ApplyConfig(cfg.transform);
	id_ = cfg.guid;
	parentId_ = cfg.parentGuid;
	name_ = cfg.name;
}

void BaseGameObject::ExtractConfig(){
	BaseGameObjectConfig& cfg = config_.GetConfig();

	if (model_) cfg.modelConfig = model_->ExtractConfig();
	if (collider_) cfg.colliderConfig = collider_->ExtractConfig();
	cfg.transform = worldTransform_.ExtractConfig();
	cfg.objectType = static_cast< int >(objectType_);
	cfg.name = name_;
	cfg.guid = id_;
	cfg.parentGuid = parentId_;
}

void BaseGameObject::ApplyConfigFromJson(const nlohmann::json& j){
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void BaseGameObject::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast< BaseGameObject* >(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

//===================================================================*/
//                   getter/setter
//===================================================================*/

void BaseGameObject::SetName(const std::string& name){
	SceneObject::SetName(name, ObjectType::GameObject);
}

void BaseGameObject::SetTranslate(const Vector3& pos){
	if (model_){
		worldTransform_.translation = pos;
	}
}

void BaseGameObject::SetScale(const Vector3& scale){
	if (model_){
		worldTransform_.scale = scale;
	}
}

void BaseGameObject::SetDrawEnable(bool isDrawEnable){
	SceneObject::SetDrawEnable(isDrawEnable);
	model_->SetIsDrawEnable(isDrawEnable);
}

const Vector3 BaseGameObject::GetCenterPos()const{
	const Vector3 offset = {0.0f, 0.5f, 0.0f};
	Vector3 worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

void BaseGameObject::SetColor(const Vector4& color){
	if (model_){
		model_->SetColor(color);
	}
}

void BaseGameObject::SetCollider(std::unique_ptr<Collider> collider){
	collider_ = std::move(collider);
}

Collider* BaseGameObject::GetCollider(){ return collider_.get(); }

Model* BaseGameObject::GetStaticModel(){
	return (objectModelType_ == ObjectModelType::ModelType_Static)
		? static_cast< Model* >(model_.get()) : nullptr;
}

AnimationModel* BaseGameObject::GetAnimationModel(){
	return (objectModelType_ == ObjectModelType::ModelType_Animation)
		? static_cast< AnimationModel* >(model_.get()) : nullptr;
}

const AnimationModel* BaseGameObject::GetAnimationModel() const{
	return (objectModelType_ == ObjectModelType::ModelType_Animation)
		? static_cast< AnimationModel* >(model_.get()) : nullptr;
}
//===================================================================*/
//                    load/save
//===================================================================*/
//void BaseGameObject::SaveToJson(const std::string& fileName) const{}
//
//void BaseGameObject::LoadFromJson(const std::string& fileName){}

REGISTER_SCENE_OBJECT(BaseGameObject)