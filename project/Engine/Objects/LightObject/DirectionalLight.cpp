#include <Engine/Objects/LightObject/DirectionalLight.h>

/* engine */
#include "Engine/Application/UI/Panels/InspectorPanel.h"
#include "Engine/Editor/LevelEditor.h"
#include "Engine/Foundation/Utility/Func/CxUtils.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <Engine/foundation/Utility/FileSystem/ConfigPathResolver/ConfigPathResolver.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		ctor
/////////////////////////////////////////////////////////////////////////////////////////
DirectionalLight::DirectionalLight(const std::string& name) {
	SceneObject::SetName(name, ObjectType::Light);

	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);
	shadowParamCB_.Initialize(device);

	// 初期化
	lightData_.color	 = CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f); // ライトの色
	lightData_.direction = CalyxMath::Vector3(-0.08f, -1.0f, 0.34f);   // ライトの向き
	lightData_.intensity = 1.0f;									   // 輝度

	//// コンフィグパスの生成 preset名はdefault
	// SceneObject::SetConfigPath(ConfigPathResolver::ResolvePath(GetObjectTypeName(), GetName()));
	////コンフィグの適用
	// LoadConfig(configPath_);
	shadow_.LoadParams();

#if defined(_DEBUG) || defined(DEVELOP)
	// transformの傾きにlightのdirectionを適用(ギズモ使用するため)
	worldTransform_.eulerRotation  = lightData_.direction;
	worldTransform_.rotationSource = RotationSource::Euler; // Eulerで計算
#endif

	isEnableRaycast_ = false;
}

DirectionalLight::DirectionalLight() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	constantBuffer_.Initialize(device);
	shadowParamCB_.Initialize(device);
	shadow_.LoadParams();

	// 初期化
	lightData_.color	 = CalyxMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f); // ライトの色
	lightData_.direction = CalyxMath::Vector3(-0.08f, -1.0f, 0.34f);   // ライトの向き
	lightData_.intensity = 1.0f;									   // 輝度
#if defined(_DEBUG) || defined(DEVELOP)
	// transformの傾きにlightのdirectionを適用(ギズモ使用するため)
	worldTransform_.eulerRotation  = lightData_.direction;
	worldTransform_.rotationSource = RotationSource::Euler; // Eulerで計算
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
void DirectionalLight::UploadToGpu() {
	constantBuffer_.TransferData(lightData_);

	// ShadowParam (SerializableObject継承) から GPU用POD構造体に変換
	ShadowParamGpu gpuData{};
	gpuData.shadowRayEps = shadow_.shadowRayEps;
	gpuData.baseAngularRadius = shadow_.baseAngularRadius;
	gpuData.minShadow = shadow_.minShadow;
	gpuData.isSoft = shadow_.isSoft ? 1u : 0u;
	shadowParamCB_.TransferData(gpuData);
}

///////////////////////////////////////////////////////////////////////////////////////////
//		ライトのビュー・プロジェクション行列更新
///////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::UpdateLightVP(const AABB& sceneBounds) {

	// -------------------------
	// ライト方向（正規化）
	// -------------------------
	CalyxMath::Vector3 lightDir =
		lightData_.direction.Normalize();

	// -------------------------
	// シーン中心 & サイズ
	// -------------------------
	CalyxMath::Vector3 center =
		(sceneBounds.min_ + sceneBounds.max_) * 0.5f;

	CalyxMath::Vector3 extent =
		sceneBounds.max_ - sceneBounds.min_;

	float radius = extent.Length() * 0.5f;

	// -------------------------
	// ライト位置（シーン全体が収まる距離）
	// -------------------------
	CalyxMath::Vector3 lightPos =
		center - lightDir * (radius * 2.0f);

	lightPos.y = 4000.0f; // 高さを固定

	// -------------------------
	// up ベクトル
	// -------------------------
	CalyxMath::Vector3 up =
		(std::fabs(lightDir.y) > 0.99f)
			? CalyxMath::Vector3(0.0f, 0.0f, 1.0f)
			: CalyxMath::Vector3(0.0f, 1.0f, 0.0f);

	// -------------------------
	// View 行列
	// -------------------------
	CalyxMath::Matrix4x4 lightView =
		CalyxMath::Matrix4x4::MakeLookAt(
			lightPos,
			center,
			up);

	// -------------------------
	// AABB をライト空間に変換
	// -------------------------
	const CalyxMath::Vector3& mn = sceneBounds.min_;
	const CalyxMath::Vector3& mx = sceneBounds.max_;

	CalyxMath::Vector3 corners[8] = {
		{mn.x, mn.y, mn.z},
		{mx.x, mn.y, mn.z},
		{mn.x, mx.y, mn.z},
		{mx.x, mx.y, mn.z},
		{mn.x, mn.y, mx.z},
		{mx.x, mn.y, mx.z},
		{mn.x, mx.y, mx.z},
		{mx.x, mx.y, mx.z},
	};

	CalyxMath::Vector3 minLS(+FLT_MAX, +FLT_MAX, +FLT_MAX);
	CalyxMath::Vector3 maxLS(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for(auto& c : corners) {
		CalyxMath::Vector3 v =
			CalyxMath::Matrix4x4::Transform(c, lightView);
		minLS = CalyxMath::Vector3::Min(minLS, v);
		maxLS = CalyxMath::Vector3::Max(maxLS, v);
	}

	// -------------------------
	// Ortho パラメータ（
	// -------------------------
	const float margin = 5.0f;

	float left	 = minLS.x;
	float right	 = maxLS.x;
	float bottom = minLS.y;
	float top	 = maxLS.y;

	float nearZ = minLS.z - margin;
	float farZ	= maxLS.z + margin;

	// LH: near は正
	nearZ = (std::max)(0.1f, nearZ);

	// -------------------------
	// Projection
	// -------------------------
	CalyxMath::Matrix4x4 lightProj =
		CalyxMath::MakeOrthographicMatrixLH(
			left, right,
			bottom, top,
			nearZ, farZ);

	// -------------------------
	// World → View → Projection
	// -------------------------
	lightViewProj_ = lightView * lightProj;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		コマンドを積む
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::SetCommand(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType type) {

	uint32_t index = 0;
	if(type == PipelineType::Object3D ||
	   type == PipelineType::SkinningObject3D) {
		index = 3;
	}

	constantBuffer_.SetCommand(commandList, index);

	index = 11;
	shadowParamCB_.SetCommand(commandList, index);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグ描画
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::DrawDebug() {

	// ライトの始点（ワールド座標系での位置）
	const CalyxMath::Vector3 start = worldTransform_.GetWorldPosition();

	// ライトの向き（方向ベクトル × 長さ）
	const CalyxMath::Vector3 dir	= lightData_.direction.Normalize();
	const float				 length = 3.0f; // 可視化用の長さ
	const CalyxMath::Vector3 end	= start + dir * length;

	// 線を描く
	PrimitiveDrawer::GetInstance()->DrawLine3d(start, end, {1.0f, 1.0f, 0.0f, 1.0f});
}

/////////////////////////////////////////////////////////////////////////////////////////
//		デバッグui
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::ShowGui() {
#if defined(_DEBUG) || defined(DEVELOP)
	ImGui::Dummy(ImVec2(0.0f, 5.0f));

	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		config_.ShowGui();
		ImGui::Separator();
		GuiCmd::EndSection();
	}

	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::Object)) {
		GuiCmd::SliderFloat3("direction", lightData_.direction, -1.0f, 1.0f);
		GuiCmd::EndSection();
	}

	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		GuiCmd::ColorEdit4("color", lightData_.color);
		GuiCmd::SliderFloat("Intensity", lightData_.intensity, 0.0f, 1.0f);
		GuiCmd::EndSection();
	}

	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		shadow_.ShowGui();
	}
#endif // _DEBUG
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の適用
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::ApplyConfig() {
	const auto& cfg		 = config_.GetConfig();
	lightData_.color	 = cfg.color;
	lightData_.direction = cfg.direction;
	lightData_.intensity = cfg.intensity;
	name_				 = cfg.name;
	id_					 = cfg.guid;
	parentId_			 = cfg.parentGuid;
}

void DirectionalLight::ApplyConfigFromJson(const nlohmann::json& j) {
	config_.ApplyConfigFromJson(j);
	ApplyConfig();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		設定の吐きだし
/////////////////////////////////////////////////////////////////////////////////////////
void DirectionalLight::ExtractConfig() {
	auto& cfg	   = config_.GetConfig();
	cfg.color	   = lightData_.color;
	cfg.direction  = lightData_.direction;
	cfg.intensity  = lightData_.intensity;
	cfg.objectType = static_cast<int>(objectType_);
	cfg.name	   = name_;
	cfg.guid	   = id_;
	cfg.parentGuid = parentId_;
}

void DirectionalLight::ExtractConfigToJson(nlohmann::json& j) const {
	const_cast<DirectionalLight*>(this)->ExtractConfig();
	config_.ExtractConfigToJson(j);
}

DirectionalLight::ShadowParam::ShadowParam() {
	AddField("rayEps", shadowRayEps).Category("Shadow");
	AddField("baseAngularRadius", baseAngularRadius).Category("Shadow");
	AddField("minShadow", minShadow).Category("Shadow");
	AddField("enableSoftShadow", isSoft).Category("Shadow"); 
}

CalyxEngine::ParamPath DirectionalLight::ShadowParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Engine, "RaytracingShadow", "Graphics/Shadow"};
}

REGISTER_SCENE_OBJECT(DirectionalLight)