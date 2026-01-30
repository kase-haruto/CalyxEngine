#include <Engine/Objects/LightObject/PointLight.h>

/* engine */
#include "Engine/Application/UI/Panels/InspectorPanel.h"

#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

#ifdef _DEBUG
#include<externals/imgui/imgui.h>
#endif // _DEBUG

PointLight::PointLight(const std::string& name){
	SceneObject::SetName(name, ObjectType::Light);
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color = CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ライトの色
	lightData_.position = CalyxMath::Vector3(0.0f, 0.0f, 0.0f);		// ライトの位置
	lightData_.intensity = 0.25f;						// 光度
	lightData_.radius = 20.0f;							// 最大距離
	lightData_.decay = 1.0f;							// 減衰率

	//SceneObject::SetConfigPath(ConfigPathResolver::ResolvePath(GetObjectTypeName(), GetName()));
	//LoadConfig(configPath_);

	isEnableRaycast_ = false;
}

PointLight::PointLight(){
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color = CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// ライトの色
	lightData_.position = CalyxMath::Vector3(0.0f, 0.0f, 0.0f);		// ライトの位置
	lightData_.intensity = 0.25f;						// 光度
	lightData_.radius = 20.0f;							// 最大距離
	lightData_.decay = 1.0f;							// 減衰率

	isEnableRaycast_ = false;
}

PointLight::~PointLight(){}

void PointLight::Initialize(){}

void PointLight::Update([[maybe_unused]]float dt){}

void PointLight::AlwaysUpdate([[maybe_unused]]float dt){}

void PointLight::ShowGui(){
#if defined(_DEBUG) || defined(DEVELOP)
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
	
	// コンフィグ
	if (GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		config_.ShowGui();
		ImGui::Separator();
		GuiCmd::EndSection();
	}

	ImGui::Separator();
	
	// トランスフォーム (位置)
	if (GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Object)) {
		GuiCmd::DragFloat3("position", lightData_.position);
		GuiCmd::EndSection();
	}

	// ライトパラメータ
	if (GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		GuiCmd::ColorEdit4("color", lightData_.color);
		GuiCmd::SliderFloat("Intensity", lightData_.intensity, 0.0f, 1.0f);
		GuiCmd::DragFloat("radius", lightData_.radius);
		GuiCmd::DragFloat("decay", lightData_.decay);
		GuiCmd::EndSection();
	}
#endif // _DEBUG
}

void PointLight::UploadToGpu(){
	constantBuffer_.TransferData(lightData_);
}

void PointLight::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type){
	uint32_t index = 0;
	if(type == PipelineType::Object3D ||
	   type == PipelineType::SkinningObject3D) {
		index = 5;
	}
	constantBuffer_.SetCommand(commandList, index);
}

//===================================================================*/
 //                    config
 //===================================================================*/
void PointLight::ApplyConfig(){
	const auto& cfg = config_.GetConfig();

	// コンフィグの適用
	lightData_.color = cfg.color;
	lightData_.position = cfg.position;
	lightData_.intensity = cfg.intensity;
	lightData_.radius = cfg.radius;
	lightData_.decay = cfg.decay;
	name_ = cfg.name;
	id_ = cfg.guid;
	parentId_ = cfg.parentGuid;

}

void PointLight::ExtractConfig(){
	auto& cfg = config_.GetConfig();

	cfg.color = lightData_.color;
	cfg.position = lightData_.position;
	cfg.intensity = lightData_.intensity;
	cfg.radius = lightData_.radius;
	cfg.decay = lightData_.decay;
	cfg.name = name_;
	cfg.guid = id_;
	cfg.parentGuid = parentId_;
}

void PointLight::ApplyConfigFromJson(const nlohmann::json& j){
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

void PointLight::ExtractConfigToJson(nlohmann::json& j) const{
	const_cast< PointLight* >(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}


REGISTER_SCENE_OBJECT(PointLight)