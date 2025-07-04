#include "Player.h"

/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Foundation/Utility/Ease/Ease.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/Application/Effects/Intermediary/FxIntermediary.h>

// externals
#include <externals/imgui/imgui.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// c++
#include <numbers>

Player::Player(const std::string& modelName,
			   std::optional<std::string> objectName)
	:Actor::Actor(modelName, objectName) {
	worldTransform_.translation = { 0.0f, 0.0f, 10.0f };
	worldTransform_.scale = {1.5f,1.5f,1.5f};
	collider_->SetTargetType(ColliderType::Type_Enemy);
	collider_->SetType(ColliderType::Type_Player);


}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Initialize() {
	moveSpeed_ = 15.0f;
	InitializeEffect();
	reticleTransform_.Initialize();
	reticleTransform_.parent = &worldTransform_;
	reticleTransform_.translation = Vector3(0.0f, 0.0f, 10.0f);

	life_ = 10;

	lifeSprite_.resize(life_);
	for (size_t i = 0; i < life_; i++) {
		lifeSprite_[i] = std::make_unique<Sprite>("Textures/life.png");
		Vector2 pos = { 100.0f * i + 30.0f,50.0f };
		lifeSprite_[i]->Initialize(pos, {64.0f,64.0f});
	}

	attackSprite_ = std::make_unique<Sprite>("Textures/attackUI.png");
	Vector2 attackUiPos = Vector2(1280.0f - 200.0f, 720.0f - 200.0f);
	Vector2 attackUiSize = Vector2(128.0f,64.0f);
	attackSprite_->Initialize(attackUiPos, attackUiSize);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Update() {
	//移動
	Move();
	UpdateReticlePosition();

	shootInterval_ -= ClockManager::GetInstance()->GetDeltaTime();
	if (Input::GetInstance()->PushKey(DIK_SPACE) && shootInterval_ <= 0.0f
		||Input::GetInstance()->PushGamepadButton(PAD_BUTTON::RB) && shootInterval_ <= 0.0f) {
		Shoot();
		shootInterval_ = kMaxShootInterval_;
	}

	for (auto& sprite : lifeSprite_) {
		sprite->Update();
	}
	attackSprite_->Update();

	reticleTransform_.Update();
	bulletContainer_->Update();
	BaseGameObject::Update();
}

void Player::Draw([[maybe_unused]]ID3D12GraphicsCommandList* cmdList) {
	for (auto& sprite : lifeSprite_) {
		sprite->Draw(cmdList);
	}
	attackSprite_->Draw(cmdList);
}


/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void Player::DerivativeGui() {
	ImGui::DragFloat("moveSpeed", &moveSpeed_, 0.01f, 0.0f, 10.0f);

}


///////////////////////////////////////////////////////////////////////////////////
//		移動
///////////////////////////////////////////////////////////////////////////////////
void Player::Move() {
	Vector3 moveVector = { 0.0f, 0.0f, 0.0f };

	// キーボード移動
	if (Input::GetInstance()->PushKey(DIK_A)) {
		moveVector.x -= 1.0f;
	} else if (Input::GetInstance()->PushKey(DIK_D)) {
		moveVector.x += 1.0f;
	}

	if (Input::GetInstance()->PushKey(DIK_W)) {
		moveVector.y += 1.0f;
	} else if (Input::GetInstance()->PushKey(DIK_S)) {
		moveVector.y -= 1.0f;
	}

	// ゲームパッド左スティック入力
	Vector2 leftStick = Input::GetInstance()->GetLeftStick();
	moveVector.x += leftStick.x;
	moveVector.y += leftStick.y;

	if (moveVector.Length() > 0.0f) {
		moveVector.Normalize();
	}

	moveVector *= moveSpeed_;

	// 移動加算
	worldTransform_.translation += moveVector * ClockManager::GetInstance()->GetDeltaTime();

	UpdateTilt(moveVector);
}


void Player::Shoot() {
	Vector3 playerPos = worldTransform_.GetWorldPosition();
	Vector3 reticlePos = reticleTransform_.GetWorldPosition();

	Vector3 dir = reticlePos - playerPos;
	if (dir.Length() > 0.001f) {
		dir = dir.Normalize();
	} else {
		dir = Vector3(0.0f, 0.0f, 1.0f); // フォールバック方向
	}

	bulletContainer_->AddBullet(BulletType::Player, playerPos, dir);
}

void Player::UpdateReticlePosition() {
	constexpr float moveSpeed = 12.0f;
	float dt = ClockManager::GetInstance()->GetDeltaTime();

	Vector3 offset = Vector3::Zero();

	// キーボード入力
	if (Input::GetInstance()->PushKey(DIK_UP))    offset.y += 1.0f;
	if (Input::GetInstance()->PushKey(DIK_DOWN))  offset.y -= 1.0f;
	if (Input::GetInstance()->PushKey(DIK_LEFT))  offset.x -= 1.0f;
	if (Input::GetInstance()->PushKey(DIK_RIGHT)) offset.x += 1.0f;

	// ゲームパッドの右スティック入力を加算
	Vector2 rightStick = Input::GetInstance()->GetRightStick();
	offset.x += rightStick.x;  // 右スティック横方向
	offset.y += rightStick.y;  // 右スティック縦方向

	if (offset.Length() > 0.0f) {
		offset.Normalize();
		offset *= moveSpeed * dt;
		reticleTransform_.translation += offset;

		// 制限
		reticleTransform_.translation.x = std::clamp(reticleTransform_.translation.x, -6.0f, 6.0f);
		reticleTransform_.translation.y = std::clamp(reticleTransform_.translation.y, -3.0f, 4.0f);
		reticleTransform_.translation.z = std::clamp(reticleTransform_.translation.z, 1.0f, 20.0f);
	}
}

void Player::UpdateTilt(const Vector3& inputVector){
	// 閾値以下なら傾きを戻す
	if (inputVector.Length() <= 0.01f){
		Quaternion identity = Quaternion::MakeIdentity();
		worldTransform_.rotation = Quaternion::Slerp(worldTransform_.rotation, identity, 0.1f);
		worldTransform_.rotationSource = RotationSource::Quaternion;
		return;
	}

	Vector3 dir = inputVector.Normalize();

	// 最大角度（ラジアン）
	const float maxRoll = 0.5f;
	const float maxPitch = 0.5f;

	float targetRoll = -dir.x * maxRoll;
	float targetPitch = -dir.y * maxPitch;

	// ロールとピッチのクォータニオンを作成
	Quaternion rollQ = Quaternion::MakeRotateZ(targetRoll);
	Quaternion pitchQ = Quaternion::MakeRotateX(targetPitch);

	// 合成クォータニオン（注意：回転順序によって見た目が変わる）
	Quaternion targetRotation = Quaternion::Multiply(rollQ, pitchQ); // roll * pitch

	// なめらかに補間
	worldTransform_.rotation = Quaternion::Slerp(worldTransform_.rotation, targetRotation, 0.15f);
	worldTransform_.rotationSource = RotationSource::Quaternion;
}


///////////////////////////////////////////////////////////////////////////////////
//		バレルロール
///////////////////////////////////////////////////////////////////////////////////
float Player::EaseForwardThenReturn(float t) {
	if (t < 0.5f) {
		float x = t / 0.5f;
		return x * (2 - x); // EaseOutQuad
	} else {
		float x = (t - 0.5f) / 0.5f;
		return 1.0f - (x * x); // EaseInQuad (逆補間)
	}
}

void Player::InitializeEffect() {
}

