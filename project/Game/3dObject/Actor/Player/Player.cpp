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
#include <Game/3dObject/Actor/Player/Dodge/PlayerDodgeMotion.h>

// externals
#include "Damage/PlayerDamageHandler.h"
#include "Dodge/PlayerDodgeSystem.h"
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Game/Input/PlayerInput/PlayerInputHandler.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>

// c++

Player::Player()  = default;
Player::~Player() = default;

namespace {

inline void SetWorldPosKeepRotScale(WorldTransform& wt, const Vector3& worldPos) {
	// 親が何段でもOK：親の world は祖先込み合成
	if(wt.parent) {
		wt.parent->Update(); // 親の world を最新に
		const Matrix4x4 invParent = Matrix4x4::Inverse(wt.parent->matrix.world);
		// 位置のみをローカルへ戻す（回転・スケールはローカル既存値を維持）
		wt.translation = Vector3::Transform(worldPos, invParent);
	} else {
		wt.translation = worldPos;
	}
}

// world を「画面内の矩形（px余白あり）」に収めたワールド座標へ
inline Vector3 ClampWorldByScreenBox(const Vector3& world,
									 float marginXpx, float marginYpx) {
	auto* cam = CameraManager::GetMain3d();
	if(!cam) return world;

	// スクリーン座標と NDC z を取得
	const Matrix4x4& VP	  = cam->GetViewProjectionMatrix();
	const Vector4	 clip = Vector4::Transform(Vector4(world, 1.0f), VP);

	// 背面や極端ケースは触らない
	constexpr float kEps = 1e-6f;
	if(clip.w <= kEps) return world;

	const Vector3 ndc = {clip.x / clip.w, clip.y / clip.w, clip.z / clip.w};
	Vector2		  scr = Cx::Math::WorldToScreen(world);

	// 画面サイズと余白でクランプ（float化しておく）
	constexpr float W = static_cast<float>(kGameWidth);
	constexpr float H = static_cast<float>(kGameHeight);

	const float minX = (std::max)(0.0f, marginXpx);
	const float maxX = (std::max)(minX, W - marginXpx);
	const float minY = (std::max)(0.0f, marginYpx);
	const float maxY = (std::max)(minY, H - marginYpx);

	const Vector2 clamped = {
		std::clamp(scr.x, minX, maxX),
		std::clamp(scr.y, minY, maxY)};

	// 変化なしなら元のワールドを返す
	if(clamped.x == scr.x && clamped.y == scr.y) return world;

	return Cx::Math::ScreenToWorld(clamped, ndc.z);
}

// WorldTransform を画面内にクランプ（ローカル translation へ反映まで）
inline void ClampWorldTransformInView(WorldTransform& wt,
									  float marginXpx, float marginYpx) {
	// 自身・親の world を最新化
	wt.Update();

	const Vector3 nowW = wt.GetWorldPosition();
	const Vector3 clW  = ClampWorldByScreenBox(nowW, marginXpx, marginYpx);

	// 親が何段でもローカルへ戻して translation を更新
	SetWorldPosKeepRotScale(wt, clW);
}

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Player::Player(const std::string&		  modelName,
			   std::optional<std::string> objectName)
	: Actor::Actor(modelName, objectName) {
	worldTransform_.translation = {0.0f, 0.0f, 8.0f};
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
	Actor::SetLife(30);

	// ライフゲージの初期化
	hpGauge_ = std::make_unique<HpGauge>(static_cast<float>(life_));
	// 左下にライフゲージを設定
	Vector2 lifeGaugePos = {100.0f, 630.0f};
	hpGauge_->Initialize(lifeGaugePos, Vector2(360.0f, 32.0f));
	hpGauge_->SetAncorPoint({0.0f, 0.5f}); // 左中央

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
	if(!dodgeSystem_) {
		PlayerDodgeConfig cfg;
		cfg.useCustomCurve = true;
		cfg.lateralScale   = 0.0f;
		cfg.backwardScale  = 0.70f;
		cfg.spinTurns	   = 1.0f;

		dodgeSystem_ = std::make_unique<PlayerDodgeSystem>();
		dodgeSystem_->Initialize(this, cfg);
	}

	// 回避モーション
	if(!dodgeMotion_) {
		dodgeMotion_ = std::make_unique<PlayerDodgeMotion>();
		dodgeMotion_->Initialize(this, dodgeSystem_.get());
	}

	// ---- 危険察知 ----
	if(!danger_) {

		DangerSenseContext dangerCtx{
			// プレイヤー中心座標
			.getPlayerCenter = [this]() {
				return GetCenterPos();
			},

			// プレイヤー半径
			.getPlayerRadius = [this]() {
				return GetCollisionRadius();
			},

			// ジャスト回避ヒントの伝達先
			.setPerfectDodgeHint =
				[this](bool enable) {
					if(dodgeSystem_) {
						dodgeSystem_->SetPerfectHintActive(enable);
					}
			}
		};

		danger_ = std::make_unique<PlayerDangerSense>();
		danger_->Initialize(dangerCtx, {});
	}

	// DamageHandler
	if(!damageHandler_) {
		damageHandler_ = std::make_unique<PlayerDamageHandler>();
		damageHandler_->Initialize(this);
	}
	dodgeSystem_->SetOnRequestInvincible(
		[this](float sec) { if(damageHandler_) { damageHandler_->RequestInvincible(sec); } });

	if(!lockOn_) {
		lockOn_ = std::make_unique<PlayerLockOn>();
		lockOn_->Initialize(this);
	}

	// fx
	shootFx_ = SceneAPI::Instantiate<FxObject>("ShootFx");
	shootFx_->LoadFromPath("Effect/ShootEffect");
	auto self = shared_from_this();
	shootFx_->SetParent(self);
	shootFx_->StopAll();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Update(float dt) {
	// 入力取得
	std::vector<PlayerCommand> cmds = input_.CollectCommands(dt);

	// コマンド処理
	for(const auto& c : cmds) {
		switch(c.type) {

		case PlayerCommandType::Move:
			if(auto* m = std::get_if<CmdMove>(&c.value)) {
				AddMoveRequest(m->delta * moveSpeed_ * dt);
				UpdateTilt(m->delta);
			}
			break;

		case PlayerCommandType::MoveReticle:
			if(auto* m = std::get_if<CmdMove>(&c.value)) {
				MoveReticle(m->delta);
			}
			break;

		case PlayerCommandType::Shoot:
			RequestShoot();
			break;

		case PlayerCommandType::Dodge:
			RequestDodge();
			break;

		default:
			break;
		}
	}
	if(dodgeSystem_) {
		dodgeSystem_->Update(dt);
	}
	if(dodgeMotion_) {
		dodgeMotion_->Update(dt);
	}
	if(damageHandler_) {
		damageHandler_->Update(dt);
	}
	if(lockOn_) {
		lockOn_->Update(dt);
	}

	moveCtrler_.Apply(worldTransform_);

	if(shootingController_) {
		shootingController_->Update(dt);
	}

	if(danger_) danger_->Update(dt);

	if(hpGauge_) {
		hpGauge_->Update(dt);
		hpGauge_->SetHp(static_cast<float>(life_));
	}
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
void Player::DerivativeGui() {
	if(hpGauge_) {
		hpGauge_->ShowGui();
	}

	ImGui::DragFloat("moveSpeed", &moveSpeed_, 0.01f, 0.0f, 10.0f);
}

/////////////////////////////////////////////////////////////////////////////////////////
/// 	移動リクエストの追加
/////////////////////////////////////////////////////////////////////////////////////////
void Player::AddMoveRequest(const Vector3& delta) { moveCtrler_.AddMove(delta); }

/* ======================================================================================
/*		private functions
/* ==================================================================================== */

///////////////////////////////////////////////////////////////////////////////////
//		レティクルの移動
///////////////////////////////////////////////////////////////////////////////////
void Player::MoveReticle(const Vector3& offset) { reticleTransform_.translation += offset; }

///////////////////////////////////////////////////////////////////////////////////
//		弾の発射をりくえすと
///////////////////////////////////////////////////////////////////////////////////
void Player::RequestShoot() const {
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
	const auto& targets =
		lockOn_ ? lockOn_->GetLockedTargets()
				: std::vector<std::shared_ptr<Enemy>>{};

	shootingController_->SetTargets(targets);

	shootingController_->SetMode(targets.empty()
									 ? PlayerShoot::BulletMode::Straight
									 : PlayerShoot::BulletMode::Homing);

	// 発射されていたらエフェクトを再生
	if(bool isFired = shootingController_->RequestShoot(playerPos, dir)) {
		// 発射エフェクト
		shootFx_->PlayAll();
	}
	if(lockOn_) {
		lockOn_->RequestLockOnClear();
	}
}

///////////////////////////////////////////////////////////////////////////////////
//		ロックオン処理
///////////////////////////////////////////////////////////////////////////////////
void Player::RequestLockOn() const {
	if(lockOn_) {
		lockOn_->RequestLockOn();
	}
}

void Player::AttachDangerSenseSource(EnemyDirectory* dir) const {
	if(danger_) danger_->SetEnemyDirectory(dir);
}

void Player::RequestLockOnTargetClear() const {
	if(lockOn_) {
		lockOn_->RequestLockOnClear();
	}
}

void Player::Start() {
	if(!inputHandler_) inputHandler_ = std::make_unique<PlayerInputHandler>();

	if(!shootingController_) {
		auto bullets		= SceneAPI::Instantiate<PlayerBulletContainer>("playerBulletController");
		shootingController_ = std::make_unique<PlayerShootingController>(bullets.get());
	}
}

void Player::RequestDodge() const {
	if(dodgeSystem_) {
		dodgeSystem_->RequestDodge();
	}
}

///////////////////////////////////////////////////////////////////////////////////
//		衝突
///////////////////////////////////////////////////////////////////////////////////
void Player::OnCollisionEnter(Collider* other) {

	// イベントの場合スキップ
	if(other->GetType() == ColliderType::Type_EventObject) return;

	if(damageHandler_) {
		damageHandler_->OnHit(other);
	}
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
	if(clampReticleInView_) {
		ClampWorldTransformInView(reticleTransform_, clampMarginXpx_, clampMarginYpx_);
	}
}

/* ======================================================================================
/*		accessor
/* ==================================================================================== */
void Player::SetParent(WorldTransform* parent) { worldTransform_.parent = parent; }

void Player::AttachEnemyList(const std::list<std::shared_ptr<Enemy>>& list) const {
	if(lockOn_) {
		lockOn_->SetEnemyList(list);
	}
}

std::vector<Sprite*> Player::GetAllSprites() const {
	std::vector<Sprite*> sprites;
	for(auto& s : reticleSprites_) sprites.push_back(s.get());
	// for(auto& s : lifeSprite_) sprites.push_back(s.get());
	for(auto& s : lockOn_->GetSprites()) sprites.push_back(s);

	// ライフゲージ
	if(hpGauge_) {
		sprites.push_back(hpGauge_->GetFrameSprite());
		sprites.push_back(hpGauge_->GetDamageGauge());
		sprites.push_back(hpGauge_->GetMainGauge());
	}
	// 危険UI
	if(danger_ && danger_->GetUiSprite()) sprites.push_back(danger_->GetUiSprite());
	return sprites;
}

const Vector3 Player::GetCenterPos() const {
	const Vector3 offset   = {0.0f, 3.0f, 0.0f};
	Vector3		  worldPos = Vector3::Transform(offset, worldTransform_.matrix.world);
	return worldPos;
}

std::optional<float> Player::GetShootCooldown() const {
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

REGISTER_SCENE_OBJECT(Player)