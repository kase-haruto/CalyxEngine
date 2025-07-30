#include <Engine/Objects/LightObject/DirectionalLight.h>

/* engine */
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

/* externals */
#ifdef _DEBUG
#include<externals/imgui/imgui.h>
#endif // _DEBUG


DirectionalLight::DirectionalLight(const std::string& name){
	SceneObject::SetName(name, ObjectType::Light);

	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ライトの色
	lightData_.direction = Vector3(0.0f, -1.0f, 0.0f);	// ライトの向き
	lightData_.intensity = 0.25f;						// 輝度

	//// コンフィグパスの生成 preset名はdefault
	//SceneObject::SetConfigPath(ConfigPathResolver::ResolvePath(GetObjectTypeName(), GetName()));
	////コンフィグの適用
	//LoadConfig(configPath_);

	isEnableRaycast_ = false;
}

DirectionalLight::DirectionalLight(){
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ライトの色
	lightData_.direction = Vector3(0.0f, -1.0f, 0.0f);	// ライトの向き
	lightData_.intensity = 0.25f;						// 輝度

	isEnableRaycast_ = false;
}

DirectionalLight::~DirectionalLight(){}

void DirectionalLight::Initialize(){}

void DirectionalLight::Update([[maybe_unused]] float dt){}

void DirectionalLight::AlwaysUpdate([[maybe_unused]]float dt){}

void DirectionalLight::UploadToGpu(){
	constantBuffer_.TransferData(lightData_);
}

void DirectionalLight::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type){

	uint32_t index = 0;
	if (type == PipelineType::Object3D || PipelineType::SkinningObject3D){
		index = 3;
	}

	constantBuffer_.SetCommand(commandList, index);
}

void DirectionalLight::ShowGui(){
#ifdef _DEBUG
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	config_.ShowGui();

	ImGui::Separator();

	GuiCmd::SliderFloat3("direction", lightData_.direction, -1.0f, 1.0f);
	GuiCmd::ColorEdit4("color", lightData_.color);
	GuiCmd::SliderFloat("Intensity", lightData_.intensity, 0.0f, 1.0f);
#endif // _DEBUG
}

void DirectionalLight::ApplyConfig(){
	const auto& cfg = config_.GetConfig();
	lightData_.color = cfg.color;
	lightData_.direction = cfg.direction;
	lightData_.intensity = cfg.intensity;
	name_ = cfg.name;
	id_ = cfg.guid;
	parentId_ = cfg.parentGuid;
}

void DirectionalLight::ExtractConfig(){
	auto& cfg = config_.GetConfig();
	cfg.color = lightData_.color;
	cfg.direction = lightData_.direction;
	cfg.intensity = lightData_.intensity;
	cfg.objectType = static_cast< int >(objectType_);
	cfg.name = name_;
	cfg.guid = id_;
	cfg.parentGuid = parentId_;
}

void DirectionalLight::ApplyConfigFromJson(const nlohmann::json& j){
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void DirectionalLight::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast< DirectionalLight* >(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

REGISTER_SCENE_OBJECT(DirectionalLight)