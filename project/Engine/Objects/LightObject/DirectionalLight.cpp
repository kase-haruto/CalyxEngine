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

void DirectionalLight::Initialize() {}

void DirectionalLight::Update(){
	
}

void DirectionalLight::UploadToGpu(){
	constantBuffer_.TransferData(lightData_);
}

void DirectionalLight::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,PipelineType type){
	
	uint32_t index = 0;
	if (type == PipelineType::Object3D||PipelineType::SkinningObject3D){
		index = 3;
	}

	constantBuffer_.SetCommand(commandList, index);
}

void DirectionalLight::ShowGui(){
#ifdef _DEBUG
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	if (ImGui::Button("Save Config")) {
		SaveConfig(GetConfigPath());
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Config")) {
		LoadConfig(GetConfigPath());
	}

	ImGui::Separator();

	GuiCmd::SliderFloat3("direction", lightData_.direction, -1.0f, 1.0f);
	GuiCmd::ColorEdit4("color", lightData_.color);
	GuiCmd::SliderFloat("Intensity", lightData_.intensity, 0.0f, 1.0f);
#endif // _DEBUG
}

void DirectionalLight::ApplyConfig() {
	lightData_.color = config_.color;
	lightData_.direction = config_.direction;
	lightData_.intensity = config_.intensity;
	name_ = config_.name;
	id_ = config_.guid;
	parentId_ = config_.parentGuid;
}

void DirectionalLight::ExtractConfig(){
	config_.color = lightData_.color;
	config_.direction = lightData_.direction;
	config_.intensity = lightData_.intensity;
	config_.objectType = static_cast<int>(objectType_);
	config_.name = name_;
	config_.guid = id_;
	config_.parentGuid = parentId_;
}

REGISTER_SCENE_OBJECT(DirectionalLight)