#include "TutorialEvent.h"
/* ========================================================================
/*      include space
/* ===================================================================== */
#include "Engine/Application/System/CalyxCore.h"
#include "Engine/Objects/Collider/BoxCollider.h"
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

/////////////////////////////////////////////////////////////////////////
//		コンストラクタ / デストラクタ
/////////////////////////////////////////////////////////////////////////
TutorialEvent::TutorialEvent() {
	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider");   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
}

TutorialEvent::TutorialEvent(const std::string& name) {
	SceneObject::SetName(name,ObjectType::Event);
	
	// 衝突の設定(boxで初期化
	std::unique_ptr<BoxCollider> box = std::make_unique<BoxCollider>(true);
	box->SetName(GetName() + "BoxCollider");   //< コライダー名前設定
	box->Initialize(CalyxMath::Vector3(1.0f)); //< サイズ設定
	collider_ = std::move(box);
	collider_->SetType(ColliderType::Type_EventObject);
	collider_->SetTargetType(ColliderType::Type_Player);

	collider_->SetOnEnter([this](Collider* other) { this->OnCollisionEnter(other); });
	collider_->SetOnStay([this](Collider* other) { this->OnCollisionStay(other); });
	collider_->SetOnExit([this](Collider* other) { this->OnCollisionExit(other); });
}

TutorialEvent::~TutorialEvent() = default;

/////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::Initialize() {
	// 色を黄色に設定する
}

/////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::AlwaysUpdate([[maybe_unused]]float dt) {
	worldTransform_.Update();

	CalyxMath::Vector3	  worldPos = worldTransform_.GetWorldPosition();
	CalyxMath::Quaternion rot	   = worldTransform_.rotation;

	// collider の更新
	if(collider_) {
		if(collider_->IsCollisionEnubled()) {
			collider_->Update(worldPos, rot);
			auto* box = dynamic_cast<BoxCollider*>(collider_.get());
			if(box) box->SetSize(worldTransform_.scale);
			collider_->Draw();
		}
	}
}

/////////////////////////////////////////////////////////////////////////
//		imgui/ui
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::ShowGui() {
	if(GuiCmd::BeginSection(CalyxEditor::ParamFilterSection::ParameterData)) {
		// NOTE: イベントのパラメータをここで表示 
	}
}

/////////////////////////////////////////////////////////////////////////
//		イベント発火時
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::OnCollisionEnter(Collider* other) {
	(void)other;
}

/////////////////////////////////////////////////////////////////////////
//		イベント継続中の処理
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::OnCollisionStay(Collider* other) {
	(void)other;
}

/////////////////////////////////////////////////////////////////////////
//		イベント終了時の処理
/////////////////////////////////////////////////////////////////////////
void TutorialEvent::OnCollisionExit(Collider* other) {
	(void)other;
}
