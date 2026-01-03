#include "ClearLogoHud.h"

#include "Engine/Application/System/Enviroment.h"
#include "Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h"

ClearLogoHud::ClearLogoHud()  = default;
ClearLogoHud::~ClearLogoHud() = default;

///////////////////////////////////////////////////////////////////////////////////////////////
//		初期化
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::Initialize() {
	// 外部出力用パラメータ設定
	InitializeSerializableParm();

	// config構築
	CreateConfig();

	BaseHud::Initialize(static_cast<uint32_t>(Calyx2D::HudMotionChannel::Position));
	Sprite().SetScale(scale_);

}

///////////////////////////////////////////////////////////////////////////////////////////////
//		GUI表示
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::ShowGui() {
	ImGui::SeparatorText("ClearLogoHud");
	SerializableObject::SaveAndLoadButtonGui();
	GuiCmd::DragFloat2("startPosition",startPosition_);
	GuiCmd::DragFloat2("stayPosition",stayPosition_);
	GuiCmd::DragFloat("duration",duration_,0.1f);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		パラメータパス取得
///////////////////////////////////////////////////////////////////////////////////////////////
CalyxEngine::ParamPath ClearLogoHud::GetParamPath() const { return {CalyxEngine::ParamDomain::Game,"ClearLogoHud"}; }


///////////////////////////////////////////////////////////////////////////////////////////////
//		コンフィグ作成
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::CreateConfig() {

	config_.texturePath = "Textures/white1x1.png";

	// ===============================
	// 登場モーション（王道・確実）
	// ===============================

	config_.enterMotion.position = Calyx2D::HudMotionDesc<CalyxMath::Vector2>{
			.start = startPosition_,
			.end = stayPosition_,
			.duration = duration_,
			.easing = CalyxEase::EaseType::EaseOutCubic
		};

	// ===============================
	// 退場モーション（
	// ===============================
	config_.exitMotion.position = Calyx2D::HudMotionDesc<CalyxMath::Vector2>{
			.start = stayPosition_,
			.end = startPosition_,
			.duration = duration_,
			.easing = CalyxEase::EaseType::EaseInCubic
		};
}

///////////////////////////////////////////////////////////////////////////////////////////////
//		シリアライズ可能パラメータ初期化
///////////////////////////////////////////////////////////////////////////////////////////////
void ClearLogoHud::InitializeSerializableParm() {
	SerializableObject::AddField("startPosition",startPosition_);
	SerializableObject::AddField("stayPosition",stayPosition_);
	SerializableObject::AddField("scale",scale_);
	SerializableObject::AddField("duration",duration_);
	SerializableObject::LoadParams();
}