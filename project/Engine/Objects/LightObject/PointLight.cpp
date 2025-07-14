#include <Engine/Objects/LightObject/PointLight.h>

/* engine */
#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#ifdef _DEBUG
#include<externals/imgui/imgui.h>
#endif // _DEBUG



PointLight::PointLight(const std::string& name){
	SceneObject::SetName(name, ObjectType::Light);
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ライトの色
	lightData_.position = Vector3(0.0f, 0.0f, 0.0f);		// ライトの位置
	lightData_.intensity = 0.25f;						// 光度
	lightData_.radius = 20.0f;							// 最大距離
	lightData_.decay = 1.0f;							// 減衰率

	//SceneObject::SetConfigPath(ConfigPathResolver::ResolvePath(GetObjectTypeName(), GetName()));
	//LoadConfig(configPath_);

	isEnableRaycast_ = false;
}

PointLight::~PointLight(){}

void PointLight::Initialize(){}

void PointLight::Update(){
	
}

void PointLight::ShowGui(){
#ifdef _DEBUG
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	if (ImGui::Button("SaveConfig")) {
		SaveConfig(configPath_);
	}
	ImGui::SameLine();
	if (ImGui::Button("LoadConfig")) {
		LoadConfig(configPath_);
	}
	ImGui::Separator();

	ImGui::Separator();
	GuiCmd::DragFloat3("position", lightData_.position );
	GuiCmd::ColorEdit4("color", lightData_.color);
	ImGui::SliderFloat("Intensity", &lightData_.intensity, 0.0f, 1.0f);
	GuiCmd::DragFloat("radius", lightData_.radius);
	GuiCmd::DragFloat("decay", lightData_.decay);
#endif // _DEBUG
}

void PointLight::UploadToGpu(){
	constantBuffer_.TransferData(lightData_);
}

void PointLight::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type){
	uint32_t index = 0;
	if (type == PipelineType::Object3D || PipelineType::SkinningObject3D){
		index = 5;
	}
	constantBuffer_.SetCommand(commandList, index);
}

//===================================================================*/
 //                    config
 //===================================================================*/
void PointLight::ApplyConfig() {
	// コンフィグの適用
	lightData_.color = config_.color;
	lightData_.position = config_.position;
	lightData_.intensity = config_.intensity;
	lightData_.radius = config_.radius;
	lightData_.decay = config_.decay;
	name_ = config_.name;
	id_ = config_.guid;
	parentId_ = config_.parentGuid;

	constantBuffer_.TransferData(lightData_);
}

void PointLight::ExtractConfig(){
	config_.color = lightData_.color;
	config_.position = lightData_.position;
	config_.intensity = lightData_.intensity;
	config_.radius = lightData_.radius;
	config_.decay = lightData_.decay;
	config_.name = name_;
	config_.guid = id_;
	config_.parentGuid = parentId_;
}


