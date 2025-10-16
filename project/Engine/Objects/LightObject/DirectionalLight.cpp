#include <Engine/Objects/LightObject/DirectionalLight.h>

/* engine */
#include "Engine/Foundation/Utility/Func/CxUtils.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor
/////////////////////////////////////////////////////////////////////////////////////////
DirectionalLight::DirectionalLight(const std::string& name) {
	SceneObject::SetName(name,ObjectType::Light);

	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color     = Vector4(1.0f,1.0f,1.0f,1.0f); // ライトの色
	lightData_.direction = Vector3(-0.08f,-1.0f,0.34f);  // ライトの向き
	lightData_.intensity = 1.0f;                         // 輝度

	//// コンフィグパスの生成 preset名はdefault
	//SceneObject::SetConfigPath(ConfigPathResolver::ResolvePath(GetObjectTypeName(), GetName()));
	////コンフィグの適用
	//LoadConfig(configPath_);

#if defined(_DEBUG) || defined(DEVELOP)
	//transformの傾きにlightのdirectionを適用(ギズモ使用するため)
	worldTransform_.eulerRotation  = lightData_.direction;
	worldTransform_.rotationSource = RotationSource::Euler; //Eulerで計算
#endif

	isEnableRaycast_ = false;

	//
}

DirectionalLight::DirectionalLight() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);

	//初期化
	lightData_.color     = Vector4(1.0f,1.0f,1.0f,1.0f); // ライトの色
	lightData_.direction = Vector3(-0.08f,-1.0f,0.34f);  // ライトの向き
	lightData_.intensity = 1.0f;                         // 輝度

#if defined(_DEBUG) || defined(DEVELOP)
	//transformの傾きにlightのdirectionを適用(ギズモ使用するため)
	worldTransform_.eulerRotation  = lightData_.direction;
	worldTransform_.rotationSource = RotationSource::Euler; //Eulerで計算
#endif
	isEnableRaycast_ = false;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		dtor
/////////////////////////////////////////////////////////////////////////////////////////
DirectionalLight::~DirectionalLight() = default;

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::Update([[maybe_unused]] float dt) {}

/////////////////////////////////////////////////////////////////////////////////////////
//		常時更新
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::AlwaysUpdate([[maybe_unused]] float dt) {}

/////////////////////////////////////////////////////////////////////////////////////////
//		gpuに転送
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::UploadToGpu() { constantBuffer_.TransferData(lightData_); }

/////////////////////////////////////////////////////////////////////////////////////////
//		コマンドを積む
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,PipelineType type) {

	uint32_t index = 0;
	if(type == PipelineType::Object3D || PipelineType::SkinningObject3D) { index = 3; }

	constantBuffer_.SetCommand(commandList,index);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ描画
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::DrawDebug() {

	// ライトの始点（ワールド座標系での位置）
	const Vector3 start = worldTransform_.GetWorldPosition();

	// ライトの向き（方向ベクトル × 長さ）
	const Vector3 dir    = lightData_.direction.Normalize();
	const float   length = 3.0f; // 可視化用の長さ
	const Vector3 end    = start + dir * length;

	// 線を描く
	PrimitiveDrawer::GetInstance()->DrawLine3d(start,end,{1.0f,1.0f,0.0f,1.0f});

}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグui
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::ShowGui() {
#if defined(_DEBUG) || defined(DEVELOP)
	ImGui::Dummy(ImVec2(0.0f,5.0f));

	config_.ShowGui();

	ImGui::Separator();

	GuiCmd::SliderFloat3("direction",lightData_.direction,-1.0f,1.0f);
	GuiCmd::ColorEdit4("color",lightData_.color);
	GuiCmd::SliderFloat("Intensity",lightData_.intensity,0.0f,1.0f);
#endif // _DEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の適用
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::ApplyConfig() {
	const auto& cfg      = config_.GetConfig();
	lightData_.color     = cfg.color;
	lightData_.direction = cfg.direction;
	lightData_.intensity = cfg.intensity;
	name_                = cfg.name;
	id_                  = cfg.guid;
	parentId_            = cfg.parentGuid;
}

void DirectionalLight::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の吐きだし
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::ExtractConfig() {
	auto& cfg      = config_.GetConfig();
	cfg.color      = lightData_.color;
	cfg.direction  = lightData_.direction;
	cfg.intensity  = lightData_.intensity;
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name       = name_;
	cfg.guid       = id_;
	cfg.parentGuid = parentId_;
}

void DirectionalLight::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<DirectionalLight*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

REGISTER_SCENE_OBJECT(DirectionalLight)