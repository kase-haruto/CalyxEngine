#include "Player.h"

/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Application/Input/Input.h>
#include <Engine/Application/System/Enviroment.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>
// game
#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/3dObject/Actor/Player/DangerSense/PlayerDangerSense.h>
#include <Game/3dObject/Actor/Player/Dodge/PlayerDodge.h>
#include <Game/3dObject/Actor/Player/Dodge/PlayerDodgeMotion.h>

// externals
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>

// c++

Player::Player()  = default;
Player::~Player() = default;

namespace {
constexpr float kHitIFrameSec  = 1.5f;
constexpr float kBlinkHz	   = 12.0f; // 点滅周波数
constexpr float kBlinkInterval = 1.0f / kBlinkHz;
} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Player::Player(const std::string&		  modelName,
			   std::optional<std::string> objectName)
	: Actor::Actor(modelName, objectName) {
	worldTransform_.translation = {0.0f, 0.0f, 10.0f};
	worldTransform_.scale		= {1.5f, 1.5f, 1.5f};
}

/* ======================================================================================
/*		public functions
/* ==================================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Initialize() {
	moveSpeed_ = 15.0f;
	reticleTransform_.Initialize();
	reticleTransform_.parent	  = &CameraManager::GetMain3d()->GetWorldTransform();
	reticleTransform_.translation = Vector3(0.0f, 0.0f, 100.0f);

	// コライダー初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_Player);
	collider_->SetTargetType(ColliderType::Type_EnemyAttack);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(true);
	if(auto* radius = dynamic_cast<SphereCollider*>(collider_.get())) {
		radius->SetRadius(1.5f);
	}
	collider_->SetOffset({0.0f, -2.0f, 0.0f});
	collider_->SetCollisionEnabled(true);

	// life関連初期化
	life_ = 10;
	lifeSprite_.resize(life_);
	for(size_t i = 0; i < life_; i++) {
		lifeSprite_[i] = std::make_unique<Sprite>("Textures/life.png");
		Vector2 pos	   = {100.0f * i + 30.0f, 50.0f};
		lifeSprite_[i]->Initialize(pos, {64.0f, 64.0f});
	}
	RefreshLifeUI();

	// spriteの初期化
	size_t spriteCount = reticleSprites_.size();
	for(size_t i = 0; i < spriteCount; ++i) {
		reticleSprites_[i] = std::make_unique<Sprite>("Textures/reticle.png");

		float	t	 = static_cast<float>(i) / (spriteCount - 1);
		float	size = std::lerp(128.0f, 16.0f, t);
		Vector2 spriteSize(size, size);

		Vector2 initPos = kGameSize * 0.5f;
		reticleSprites_[i]->Initialize(initPos, spriteSize);
		reticleSprites_[i]->SetAnchorPoint(Vector2(0.5f, 0.5f));
	}

	// ---- 回避コンポーネント ----
	if(!dodge_) {
		PlayerDodgeConfig cfg;
		cfg.useCustomCurve = true;
		cfg.lateralScale   = 0.0f;
		cfg.backwardScale  = 0.70f;
		cfg.spinTurns	   = 1.0f;
		dodge_			   = std::make_unique<PlayerDodge>();
		dodge_->Initialize(this, cfg);
	}

	// 危険察知
	if(!danger_) {
		danger_ = std::make_unique<PlayerDangerSense>();
		danger_->Initialize(this, dodge_.get(), {}); // UIやmarginは後で調整可
	}

	// 回避モーション
	if(!dodgeMotion_) {
		dodgeMotion_ = std::make_unique<PlayerDodgeMotion>();
		dodgeMotion_->Initialize(this, dodge_.get());
	}
}

void Player::RefreshLifeUI() {
	for(size_t i = 0; i < lifeSprite_.size(); ++i) {
		const bool on = (i < static_cast<size_t>(life_));
		lifeSprite_[i]->SetIsVisible(on);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Update(float dt) {
	if(inputHandler_) {
		inputHandler_->Update(*this, dt);
	}

	if(shootingController_) {
		shootingController_->Update(dt);
	}

	if(dodge_) dodge_->Update(dt);
	if(danger_) danger_->Update(dt);
	if(dodgeMotion_) dodgeMotion_->Update(dt);

	for(auto& sprite : lifeSprite_) {
		sprite->Update();
	}

	// 無敵時間
	UpdateInvincibility(dt);

	reticleTransform_.Update();

	Vector3 playerPos  = GetWorldPosition();
	Vector3 reticlePos = reticleTransform_.GetWorldPosition();

	// ── レティクル─────────────────────────
	size_t spriteCount = reticleSprites_.size();
	if(spriteCount < 2) return;

	Vector3 diff = (reticlePos - playerPos) * 0.5f;

	for(size_t i = 0; i < spriteCount; ++i) {
		float t = static_cast<float>(i) / (spriteCount - 1);

		Vector3 worldPos = playerPos + diff * t;

		// 偶奇で回転方向を変える
		float rotateSpeed	= (i % 2 == 0) ? 0.02f : -0.02f;
		float uvRotateSpeed = (i % 2 == 0) ? 0.2f : -0.2f;

		// スプライトの回転を更新
		float currentRotate = reticleSprites_[i]->GetRotation();
		reticleSprites_[i]->SetRotation(currentRotate + rotateSpeed);

		float currentUvRotate = reticleSprites_[i]->GetUvRotate();
		reticleSprites_[i]->SetUvRotate(currentUvRotate + uvRotateSpeed);

		// スクリーン座標に変換して配置
		Vector2 screenPos = Cx::Math::WorldToScreen(worldPos);
		reticleSprites_[i]->SetPosition(screenPos);
		reticleSprites_[i]->Update();
	}

	// ── ロックオンマーカー ─────────────────────────
	for(size_t i = 0; i < lockOnSprites_.size();) {
		auto& enemy = lockedOnTargets_[i];

		// 敵が死んだ・破棄されたら両方同時に消す
		if(!enemy || !enemy->GetIsAlive()) {
			lockedOnTargets_.erase(lockedOnTargets_.begin() + i);
			lockOnSprites_.erase(lockOnSprites_.begin() + i);
			continue; // erase したのでインデックスは進めない
		}

		// 位置更新（毎フレーム投影）
		Vector2 pos = Cx::Math::WorldToScreen(enemy->GetCenterPos());
		lockOnSprites_[i]->SetPosition(pos);

		// 演出：くるくる回す + 少し拡縮して点滅
		float r = lockOnSprites_[i]->GetRotation() + 0.05f;
		lockOnSprites_[i]->SetRotation(r);

		// 軽いパルス演出（0.9 ↔ 1.1）
		float scale = 1.0f + 0.1f * std::sin(r * 4.0f);
		lockOnSprites_[i]->SetSize({64.0f * scale, 64.0f * scale});

		lockOnSprites_[i]->Update();
		++i;
	}

	if(life_ <= 0) {
		isAlive_ = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		描画
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Draw([[maybe_unused]] ID3D12GraphicsCommandList* cmdList) {}

/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void Player::DerivativeGui() { ImGui::DragFloat("moveSpeed", &moveSpeed_, 0.01f, 0.0f, 10.0f); }

/* ======================================================================================
/*		private functions
/* ==================================================================================== */

///////////////////////////////////////////////////////////////////////////////////
//		playerの移動
///////////////////////////////////////////////////////////////////////////////////
void Player::MoveBy(const Vector3& delta) {
	worldTransform_.translation += delta * ClockManager::GetInstance()->GetDeltaTime();
	UpdateTilt(delta);
}

///////////////////////////////////////////////////////////////////////////////////
//		レティクルの移動
///////////////////////////////////////////////////////////////////////////////////
void Player::MoveReticle(const Vector3& offset) { reticleTransform_.translation += offset; }

///////////////////////////////////////////////////////////////////////////////////
//		弾の発射をりくえすと
///////////////////////////////////////////////////////////////////////////////////
void Player::RequestShoot() {
	Vector3 playerPos  = worldTransform_.GetWorldPosition();
	Vector3 reticlePos = reticleTransform_.GetWorldPosition();
	Vector3 dir		   = reticlePos - playerPos;

	if(dir.Length() > 0.001f) {
		dir = dir.Normalize();
	} else {
		dir = Vector3(0.0f, 0.0f, 1.0f);
	}

	if(!shootingController_) return;

	// ロックオンしていればホーミングする弾を撃つ
	shootingController_->SetTargets(lockedOnTargets_);
	shootingController_->SetMode(lockedOnTargets_.empty()
									 ? PlayerShoot::BulletMode::Straight
									 : PlayerShoot::BulletMode::Homing);

	shootingController_->RequestShoot(playerPos, dir);

	RequestLockOnTargetClear();
}

///////////////////////////////////////////////////////////////////////////////////
//		ロックオン処理
///////////////////////////////////////////////////////////////////////////////////
void Player::RequestLockOn() {
	constexpr size_t kMaxLockOn = 4;
	if(lockedOnTargets_.size() >= kMaxLockOn) return;

	// 現在有効な 3D カメラ
	Camera3d* camera = CameraManager::GetMain3d();
	if(!camera) return;

	const Vector2 reticleScreen = Cx::Math::WorldToScreen(reticleTransform_.GetWorldPosition());
	const float	  radius		= 30.0f;

	for(const auto& enemy : targets_) {
		if(!enemy) continue;
		if(std::find(lockedOnTargets_.begin(), lockedOnTargets_.end(), enemy) != lockedOnTargets_.end()) continue;

		if(!camera->IsVisible(enemy->GetWorldAABB())) continue;

		Vector2 enemyScreen = Cx::Math::WorldToScreen(enemy->GetWorldPosition());
		if((enemyScreen - reticleScreen).Length() > radius) continue;

		// ロックオン登録
		lockedOnTargets_.push_back(enemy);

		// ロックオンUI作成
		auto marker = std::make_unique<Sprite>("Textures/lockOn.png");
		marker->Initialize(enemyScreen, Vector2(64.0f, 64.0f));
		marker->SetAnchorPoint(Vector2(0.5f, 0.5f));
		lockOnSprites_.push_back(std::move(marker));

		if(lockedOnTargets_.size() >= kMaxLockOn) break;
	}
}

void Player::AttachDangerSenseSource(EnemyDirectory* dir) {
	if(danger_) danger_->SetEnemyDirectory(dir);
}

void Player::RequestLockOnTargetClear() {
	lockedOnTargets_.clear();
	lockOnSprites_.clear();
}

void Player::Start() {
	if(!inputHandler_) inputHandler_ = std::make_unique<PlayerInputHandler>();

	if(!shootingController_) {
		auto bullets		= SceneAPI::Instantiate<PlayerBulletContainer>("playerBulletController");
		shootingController_ = std::make_unique<PlayerShootingController>(bullets.get());
	}
}

///////////////////////////////////////////////////////////////////////////////////
//		衝突
///////////////////////////////////////////////////////////////////////////////////
void Player::OnCollisionEnter(Collider* other) {

	// イベントの場合スキップ
	if(other->GetType() == ColliderType::Type_EventObject) return;
	
	// 回避のi-frameや既存の無敵ならダメージ無視
	if((dodge_ && dodge_->HandlesHitNow()) || !CanBeDamaged()) return;

	// ===== 被弾確定 =====
	--life_;

	// 被弾時にカメラを揺らす
	auto* cam		= CameraManager::GetMain3d();
	float duration	= 0.5f;
	float intensity = 0.8f;
	cam->StartShake(duration, intensity);

	RefreshLifeUI();

	// 被弾後の無敵を1秒付与
	SetInvincibleFor(kHitIFrameSec);
}

///////////////////////////////////////////////////////////////////////////////////
//		playerの傾き
///////////////////////////////////////////////////////////////////////////////////
void Player::UpdateTilt(const Vector3& inputVector) {
	Camera3d* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 閾値以下で戻す
	if(inputVector.Length() <= 0.01f) {
		Quaternion identity			   = Quaternion::MakeIdentity();
		worldTransform_.rotation	   = Quaternion::Slerp(worldTransform_.rotation, identity, 0.1f);
		worldTransform_.rotationSource = RotationSource::Quaternion;

		// カメラ傾きを戻す（オイラー角）
		Vector3 currentRot = cam->GetRotate();
		currentRot.x	   = Cx::Math::Lerp(currentRot.x, 0.0f, 0.1f); // pitch
		currentRot.z	   = Cx::Math::Lerp(currentRot.z, 0.0f, 0.1f); // roll
		cam->SetCamera(cam->GetTranslate(), currentRot);
		return;
	}

	Vector3 dir = inputVector.Normalize();

	const float maxRoll	 = 0.3f;
	const float maxPitch = 0.3f;

	float targetRoll  = -dir.x * maxRoll;
	float targetPitch = -dir.y * maxPitch;

	// プレイヤー回転（Quaternion）
	Quaternion rollQ		  = Quaternion::MakeRotateZ(targetRoll);
	Quaternion pitchQ		  = Quaternion::MakeRotateX(targetPitch);
	Quaternion targetRotation = Quaternion::Multiply(rollQ, pitchQ);

	worldTransform_.rotation	   = Quaternion::Slerp(worldTransform_.rotation, targetRotation, 0.15f);
	worldTransform_.rotationSource = RotationSource::Quaternion;

	// カメラ回転（Euler）
	Vector3 currentRot = cam->GetRotate();
	currentRot.x	   = Cx::Math::Lerp(currentRot.x, targetPitch * 0.3f, 0.15f); // pitch
	currentRot.z	   = Cx::Math::Lerp(currentRot.z, targetRoll * 0.3f, 0.15f);  // roll
	cam->SetCamera(cam->GetTranslate(), currentRot);
}

///////////////////////////////////////////////////////////////////////////////////
//		移動
///////////////////////////////////////////////////////////////////////////////////
void Player::Move() {
	Vector3 moveVector = {0.0f, 0.0f, 0.0f};

	// キーボード移動
	if(Input::GetInstance()->PushKey(DIK_A)) {
		moveVector.x -= 1.0f;
	} else if(Input::GetInstance()->PushKey(DIK_D)) {
		moveVector.x += 1.0f;
	}

	if(Input::GetInstance()->PushKey(DIK_W)) {
		moveVector.y += 1.0f;
	} else if(Input::GetInstance()->PushKey(DIK_S)) {
		moveVector.y -= 1.0f;
	}

	// ゲームパッド左スティック入力
	Vector2 leftStick = Input::GetInstance()->GetLeftStick();
	moveVector.x += leftStick.x;
	moveVector.y += leftStick.y;

	if(moveVector.Length() > 0.0f) {
		moveVector.Normalize();
	}

	moveVector *= moveSpeed_;

	// 移動加算
	worldTransform_.translation += moveVector * ClockManager::GetInstance()->GetDeltaTime();

	UpdateTilt(moveVector);
}

///////////////////////////////////////////////////////////////////////////////////
//		弾の発射
///////////////////////////////////////////////////////////////////////////////////
void Player::Shoot() {
	Vector3 playerPos  = worldTransform_.GetWorldPosition();
	Vector3 reticlePos = reticleTransform_.GetWorldPosition();

	Vector3 dir = reticlePos - playerPos;
	if(dir.Length() > 0.001f) {
		dir = dir.Normalize();
	} else {
		dir = Vector3(0.0f, 0.0f, 1.0f); // フォールバック方向
	}

	shootingController_->RequestShoot(playerPos, dir);
}

///////////////////////////////////////////////////////////////////////////////////
//		レティクルの座標更新
///////////////////////////////////////////////////////////////////////////////////
void Player::UpdateReticlePosition() {
	constexpr float moveSpeed		 = 6.0f;
	constexpr float stickSensitivity = 300.0f; // スティック感度を大きめに
	float			dt				 = ClockManager::GetInstance()->GetDeltaTime();

	Vector3 offset = Vector3::Zero();

	// キーボード入力
	if(Input::GetInstance()->PushKey(DIK_UP)) offset.y += 3.0f;
	if(Input::GetInstance()->PushKey(DIK_DOWN)) offset.y -= 3.0f;
	if(Input::GetInstance()->PushKey(DIK_LEFT)) offset.x -= 3.0f;
	if(Input::GetInstance()->PushKey(DIK_RIGHT)) offset.x += 3.0f;

	// ゲームパッド右スティック
	Vector2 rightStick = Input::GetInstance()->GetRightStick();

	// スティック感度を別で調整
	offset.x += rightStick.x * stickSensitivity * dt;
	offset.y += rightStick.y * stickSensitivity * dt;

	// キーボードだけ正規化
	Vector3 keyboardOffset = offset;
	keyboardOffset.x -= rightStick.x * stickSensitivity * dt;
	keyboardOffset.y -= rightStick.y * stickSensitivity * dt;

	if(keyboardOffset.Length() > 0.0f) {
		keyboardOffset.Normalize();
		keyboardOffset *= moveSpeed * dt;
		offset.x = keyboardOffset.x + rightStick.x * stickSensitivity * dt;
		offset.y = keyboardOffset.y + rightStick.y * stickSensitivity * dt;
	}

	reticleTransform_.translation += offset;

	// 制限
	// reticleTransform_.translation.x = std::clamp(reticleTransform_.translation.x, -6.0f, 6.0f);
	// reticleTransform_.translation.y = std::clamp(reticleTransform_.translation.y, -3.0f, 4.0f);
	// reticleTransform_.translation.z = std::clamp(reticleTransform_.translation.z, 1.0f, 20.0f);
}

/* ======================================================================================
/*		accessor
/* ==================================================================================== */
void Player::SetParent(WorldTransform* parent) { worldTransform_.parent = parent; }

std::vector<Sprite*> Player::GetAllSprites() {
	std::vector<Sprite*> sprites;
	for(auto& s : reticleSprites_) sprites.push_back(s.get());
	for(auto& s : lifeSprite_) sprites.push_back(s.get());
	for(auto& s : lockOnSprites_) sprites.push_back(s.get());

	// 危険UI
	if(danger_ && danger_->GetUiSprite()) sprites.push_back(danger_->GetUiSprite());
	return sprites;
}

const Vector3 Player::GetCenterPos() const {
	const Vector3 offset   = {0.0f, 3.0f, 0.0f};
	Vector3		  worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

std::optional<float> Player::GetShootCooldown() {
	if(shootingController_) {
		return shootingController_->GetCooldown();
	}

	return std::nullopt;
}

std::optional<const float> Player::GetMaxShootInterval() const {
	if(shootingController_) {
		return shootingController_->GetInterval();
	}

	return std::nullopt;
}

void Player::SetShootingController(std::unique_ptr<PlayerShootingController> sc) { shootingController_ = std::move(sc); }

void Player::SetInputHandler(std::unique_ptr<PlayerInputHandler> ih) { inputHandler_ = std::move(ih); }

void Player::SetInvincibleFor(float seconds) {
	if(seconds <= 0.0f) return;

	const bool wasInvincible = IsInvincible();
	invincibleTimer_		 = (std::max)(invincibleTimer_, seconds);

	if(!wasInvincible) {
		// 無敵開始
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = false;
		SetDrawEnable(false);
	}
}

bool Player::IsInvincible() const {
	return invincibleTimer_ > 0.0f;
}

void Player::UpdateInvincibility(float dt) {
	if(invincibleTimer_ <= 0.0f) return;

	invincibleTimer_ -= dt;
	if(invincibleTimer_ <= 0.0f) {
		// 無敵終了
		invincibleTimer_	  = 0.0f;
		invincibleBlinkAccum_ = 0.0f;
		invincibleBlinkState_ = true;
		SetDrawEnable(true);
		return;
	}

	// 無敵中は一定間隔で描画トグル
	invincibleBlinkAccum_ += dt;
	while(invincibleBlinkAccum_ >= kBlinkInterval) {
		invincibleBlinkAccum_ -= kBlinkInterval;
		invincibleBlinkState_ = !invincibleBlinkState_;
		SetDrawEnable(invincibleBlinkState_);
	}
}

REGISTER_SCENE_OBJECT(Player)