#include "Reticle.h"

#include "Engine/Foundation/Input/Input.h"
#include "Engine/Foundation/Utility/Animation/SimpleAnimChannel.h"
#include "Engine/Graphics/Camera/Manager/CameraManager.h"
#include "Engine/Renderer/Sprite/Sprite.h"

#include <Engine/Renderer/Sprite/SpriteRenderer.h>

Reticle::Reticle() {
	reticleSprite_ = std::make_unique<Calyx2D::SpriteObject2d>();
	param_.LoadParams();
}

Reticle::~Reticle() = default;

//////////////////////////////////////////////////////////////////////////////
//	初期化
//////////////////////////////////////////////////////////////////////////////
void Reticle::Initialize() {
	reticleSprite_->Initialize(reticleTexturePath_);
	reticleSprite_->SetPosition({400.0f,300.0f});
	reticleSprite_->GetSprite()->SetAnchorPoint(param_.spriteParam_.anchorPoint);
	reticleSprite_->SetScale(param_.spriteParam_.scale);
	transform_.Initialize();
	SetParent(&CameraManager::GetMain3d()->GetWorldTransform());
}

//////////////////////////////////////////////////////////////////////////////
//	更新
//////////////////////////////////////////////////////////////////////////////
void Reticle::Update(float dt) {
	// 移動反映
	ApplyMove(dt);
	transform_.Update();
	// 描画用スプライトに適用
	reticleSprite_->SetPosition(CalyxMath::WorldToScreen(transform_.GetWorldPosition()));
	// スプライト更新
	if(reticleSprite_) { reticleSprite_->Update(dt); }
}

//////////////////////////////////////////////////////////////////////////////
//	描画
//////////////////////////////////////////////////////////////////////////////
void Reticle::Draw(SpriteRenderer* renderer) const { reticleSprite_->Draw(renderer); }

//////////////////////////////////////////////////////////////////////////////
//	デバッグGUI
//////////////////////////////////////////////////////////////////////////////
void Reticle::ShowGui() {
	// transform 表示
	transform_.ShowImGui("Reticle Transform");

	param_.ShowGui();
}

void Reticle::SetParent(WorldTransform* transform) { transform_.parent = transform; }

//////////////////////////////////////////////////////////////////////////////
//	レティクル座標を取得
//////////////////////////////////////////////////////////////////////////////
const CalyxMath::Vector2& Reticle::GetPosition() const { return reticleSprite_->GetPosition(); }
CalyxMath::Vector3        Reticle::GetPosition3D() const { return transform_.GetWorldPosition(); }

//////////////////////////////////////////////////////////////////////////////
//	レティクル座標を取得
//////////////////////////////////////////////////////////////////////////////
void Reticle::ApplyMove(float dt) {
	auto* input = CalyxFoundation::Input::GetInstance();

	// 右スティック入力（-1 ～ 1）
	CalyxMath::Vector2 rs = input->GetRightStick();

	// 微少入力の揺れ対策（必要に応じて調整）
	const float stickDeadZone = 0.15f;
	const float mag           = std::sqrt(rs.x * rs.x + rs.y * rs.y);
	if(mag < stickDeadZone) {
		return; // 動かさない
	}

	// deadZone を抜けた部分を 0..1 に再マップ（滑らかに動く）
	const float t     = (mag - stickDeadZone) / (1.0f - stickDeadZone);
	const float scale = std::clamp(t,0.0f,1.0f) / mag;

	CalyxMath::Vector2 dir = {rs.x * scale,rs.y * scale};

	// speed（パラメータ）と dt の適用
	CalyxMath::Vector2 offset = dir * param_.speed * dt;

	transform_.translation.x += 10 * offset.x;
	transform_.translation.y += 10 * offset.y;
	transform_.translation.z = param_.posFar;
}

//////////////////////////////////////////////////////////////////////////////
//	パラメータ
//////////////////////////////////////////////////////////////////////////////
Reticle::ReticleParam::ReticleParam() {
	AddField("posFar",posFar).Category("reticle").Range(100.0f,5000.0f);
	AddField("speed",speed).Category("reticle").Range(5.0f,30.0f);
	AddField("anchorPoint",spriteParam_.anchorPoint).Category("reticle");
	AddField("scale",spriteParam_.scale).Category("reticle");
}

//////////////////////////////////////////////////////////////////////////////
//	パラメータのパス
//////////////////////////////////////////////////////////////////////////////
CalyxEngine::ParamPath Reticle::ReticleParam::GetParamPath() const { return {CalyxEngine::ParamDomain::Game,"Reticle","Player/Reticle"}; }