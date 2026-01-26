#include "Player.h"

/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Application/System/Environment.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Registry/SceneObjectRegistry.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Scene/Utility/SceneUtility.h>

// game
#include <Game/3dObject/Actor/Bullet/Container/PlayerBulletContainer.h>
#include <Game/3dObject/Actor/Player/DangerSense/PlayerDangerSense.h>
#include <Game/3dObject/Actor/Player/Dodge/PlayerDodgeMotion.h>

// externals
#include "Context/PlayerContextBuilder.h"
#include "Damage/PlayerDamageHandler.h"
#include "Dodge/PlayerDodgeSystem.h"
#include "Engine/Foundation/Utility/Func/CxUtils.h"
#include "Engine/Renderer/Sprite/SpriteRenderer.h"

#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <externals/imgui/imgui.h>

// c++

Player::Player()  = default;
Player::~Player() = default;

using CalyxFoundation::Input;

namespace {

	inline void SetWorldPosKeepRotScale(WorldTransform& wt, const CalyxMath::Vector3& worldPos) {
		// 親が何段でもOK：親の world は祖先込み合成
		if(wt.parent) {
			wt.parent->Update(); // 親の world を最新に
			const CalyxMath::Matrix4x4 invParent = CalyxMath::Matrix4x4::Inverse(wt.parent->matrix.world);
			// 位置のみをローカルへ戻す（回転・スケールはローカル既存値を維持）
			wt.translation = CalyxMath::Vector3::Transform(worldPos, invParent);
		} else {
			wt.translation = worldPos;
		}
	}

	// world を「画面内の矩形（px余白あり）」に収めたワールド座標へ
	inline CalyxMath::Vector3 ClampWorldByScreenBox(const CalyxMath::Vector3& world,
													float marginXpx, float marginYpx) {
		auto* cam = CameraManager::GetMain3d();
		if(!cam) return world;

		// スクリーン座標と NDC z を取得
		const CalyxMath::Matrix4x4& VP	 = cam->GetViewProjectionMatrix();
		const CalyxMath::Vector4	clip = CalyxMath::Vector4::Transform(CalyxMath::Vector4(world, 1.0f), VP);

		// 背面や極端ケースは触らない
		constexpr float kEps = 1e-6f;
		if(clip.w <= kEps) return world;

		const CalyxMath::Vector3 ndc = {clip.x / clip.w, clip.y / clip.w, clip.z / clip.w};
		CalyxMath::Vector2		 scr = CalyxMath::WorldToScreen(world);

		// 画面サイズと余白でクランプ（float化しておく）
		constexpr float W = static_cast<float>(kGameWidth);
		constexpr float H = static_cast<float>(kGameHeight);

		const float minX = (std::max)(0.0f, marginXpx);
		const float maxX = (std::max)(minX, W - marginXpx);
		const float minY = (std::max)(0.0f, marginYpx);
		const float maxY = (std::max)(minY, H - marginYpx);

		const CalyxMath::Vector2 clamped = {
			std::clamp(scr.x, minX, maxX),
			std::clamp(scr.y, minY, maxY)};

		// 変化なしなら元のワールドを返す
		if(clamped.x == scr.x && clamped.y == scr.y) return world;

		return CalyxMath::ScreenToWorld(clamped, ndc.z);
	}

	// WorldTransform を画面内にクランプ（ローカル translation へ反映まで）
	inline void ClampWorldTransformInView(WorldTransform& wt,
										  float marginXpx, float marginYpx) {
		// 自身・親の world を最新化
		wt.Update();

		const CalyxMath::Vector3 nowW = wt.GetWorldPosition();
		const CalyxMath::Vector3 clW  = ClampWorldByScreenBox(nowW, marginXpx, marginYpx);

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
	moveSpeed_ = param_.moveSpeed;

	// コライダー初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_Player);
	collider_->SetTargetType(ColliderType::Type_EnemyAttack);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(true);
	if(auto* radius = dynamic_cast<SphereCollider*>(collider_.get())) {
		radius->SetRadius(param_.col.radius);
	}
	collider_->SetOffset(param_.col.offset);
	collider_->SetCollisionEnabled(true);

	// life関連初期化
	Actor::SetLife(param_.life);

	// ライフゲージの初期化
	hpGauge_ = std::make_unique<HpGauge>(static_cast<float>(life_));
	// ライフゲージを設定
	hpGauge_->Initialize(param_.hp.pos, param_.hp.size);
	hpGauge_->SetAncorPoint({0.0f, 0.5f}); // 左中央

	// reticle
	reticle_ = std::make_unique<Reticle>();
	reticle_->Initialize();

	// context 構築
	PlayerContextBuilder ctxBuilder = PlayerContextBuilder(*this);

	if(!shootingController_) {
		auto bullets		= SceneAPI::Instantiate<PlayerBulletContainer>("playerBulletController");
		shootingController_ = std::make_unique<PlayerShootingController>(bullets.get());
	}

	// ---- 回避コンポーネント ----
	if(!dodgeSystem_) {
		dodgeSystem_ = std::make_unique<PlayerDodgeSystem>();
		dodgeSystem_->Initialize();
	}

	// 回避モーション
	if(!dodgeMotion_) {
		dodgeMotion_ = std::make_unique<PlayerDodgeSpinMotion>();
		dodgeMotion_->Initialize(dodgeSystem_.get(), &worldTransform_);
	}

	// ---- 危険察知 ----
	if(!danger_) {

		danger_ = std::make_unique<PlayerDangerSense>();
		danger_->Initialize(ctxBuilder.BuildState());
	}

	// DamageHandler
	if(!damageHandler_) {
		damageHandler_ = std::make_unique<PlayerDamageHandler>();
		damageHandler_->Initialize(ctxBuilder.BuildState());
	}
	dodgeSystem_->SetOnRequestInvincible(
		[this](float sec) { if(damageHandler_) { damageHandler_->RequestInvincible(sec); } });

	if(!lockOn_) {
		lockOn_ = std::make_unique<PlayerLockOn>();
		lockOn_->Initialize(ctxBuilder.BuildAction());
	}

	// fx
	shootFx_ = SceneAPI::Instantiate<CalyxEffect::FxObject>("ShootFx");
	shootFx_->LoadFromPath("Effect/ShootEffect");
	auto self = shared_from_this();
	shootFx_->SetParent(self);
	shootFx_->StopAll();

	MakeSerializableParam();
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
			// 回避中は移動入力を受け付けない
			if(dodgeSystem_ && dodgeSystem_->IsDodging()) break;

			if(auto* m = std::get_if<CmdMove>(&c.value)) {
				AddMoveRequest(m->delta * moveSpeed_ * dt);
				UpdateTilt(m->delta);
			}
			break;

		case PlayerCommandType::Shoot:
			RequestShoot();
			break;

		case PlayerCommandType::Dodge:
			if(auto* d = std::get_if<CmdDodge>(&c.value)) {
				RequestDodge(d->dir);
			} else {
				RequestDodge();
			}
			break;

		default:
			break;
		}
	}
	if(dodgeSystem_) {
		dodgeSystem_->Update(dt);
		// 回避移動
		if(dodgeSystem_->IsDodging()) {
			AddMoveRequest(dodgeSystem_->GetDodgeVelocity() * dt);
		}
	}
	if(dodgeMotion_) {
		dodgeMotion_->Update(dt);
	}
	if(damageHandler_) {
		damageHandler_->Update(dt);
	}
	if(lockOn_) {
		lockOn_->Update(dt);
		if(reticle_) {
			reticle_->SetLockedEnemyList(lockOn_->GetLockedTargets());
		}
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

	// ── レティクル─────────────────────────
	reticle_->Update(dt);

	if(life_ <= 0) {
		isAlive_ = false;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		描画
/////////////////////////////////////////////////////////////////////////////////////////
void Player::DrawHud(SpriteRenderer* spriteRenderer) {
	// for(auto& s : reticleSprites_) sprites.push_back(s.get());
	//  for(auto& s : lifeSprite_) sprites.push_back(s.get());
	for(auto& s : lockOn_->GetSprites()) {
		spriteRenderer->Register(s);
	}

	// ライフゲージ
	if(hpGauge_) {
		hpGauge_->Draw(spriteRenderer);
	}

	// レティクル
	if(reticle_) {
		reticle_->Draw(spriteRenderer);
	}

	// 危険UI
	if(danger_ && danger_->GetUiSprite()) {
		spriteRenderer->Register(danger_->GetUiSprite());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void Player::DerivativeGui() {
	if(lockOn_) {
		lockOn_->ShowGui();
	}

	if(dodgeSystem_) {
		dodgeSystem_->ShowGui();
	}

	if(danger_) {
		danger_->ShowGui();
	}

	if(damageHandler_) {
		damageHandler_->ShowGUi();
	}

	if(reticle_) {
		reticle_->ShowGui();
	}

	if(shootingController_) {
		shootingController_->ShowGui();
	}

	param_.ShowGui();
}

/////////////////////////////////////////////////////////////////////////////////////////
/// 	移動リクエストの追加
/////////////////////////////////////////////////////////////////////////////////////////
void Player::AddMoveRequest(const CalyxMath::Vector3& delta) { moveCtrler_.AddMove(delta); }

/* ======================================================================================
/*		private functions
/* ==================================================================================== */

///////////////////////////////////////////////////////////////////////////////////
//		弾の発射をりくえすと
///////////////////////////////////////////////////////////////////////////////////
void Player::RequestShoot() const {
	CalyxMath::Vector3 playerPos  = worldTransform_.GetWorldPosition();
	CalyxMath::Vector3 reticlePos = reticle_->GetPosition3D();
	CalyxMath::Vector3 dir		  = reticlePos - playerPos;

	if(dir.Length() > 0.001f) {
		dir = dir.Normalize();
	} else {
		dir = CalyxMath::Vector3(0.0f, 0.0f, 1.0f);
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
	if(shootingController_->RequestShoot(playerPos, dir)) {
		// 発射エフェクト
		shootFx_->PlayAll();
		if(lockOn_) {
			lockOn_->RequestLockOnClear();
		}
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

void Player::RequestDodge(const CalyxMath::Vector3& dir) const {
	if(dodgeSystem_) {
		dodgeSystem_->RequestDodge(dir);
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
void Player::UpdateTilt(const CalyxMath::Vector3& inputVector) {
	Camera3d* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 閾値以下で戻す
	if(inputVector.Length() <= 0.01f) {
		CalyxMath::Quaternion identity = CalyxMath::Quaternion::MakeIdentity();
		worldTransform_.rotation	   = CalyxMath::Quaternion::Slerp(worldTransform_.rotation, identity, 0.1f);
		worldTransform_.rotationSource = RotationSource::Quaternion;

		// カメラ傾きを戻す（オイラー角）
		CalyxMath::Vector3 currentRot = cam->GetRotate();
		currentRot.x				  = CalyxMath::Lerp(currentRot.x, 0.0f, 0.1f); // pitch
		currentRot.z				  = CalyxMath::Lerp(currentRot.z, 0.0f, 0.1f); // roll
		cam->SetCamera(cam->GetTranslate(), currentRot);
		return;
	}

	CalyxMath::Vector3 dir = inputVector.Normalize();

	const float maxRoll	 = 0.3f;
	const float maxPitch = 0.3f;

	float targetRoll  = -dir.x * maxRoll;
	float targetPitch = -dir.y * maxPitch;

	// プレイヤー回転（CalyxMath::Quaternion）
	CalyxMath::Quaternion rollQ			 = CalyxMath::Quaternion::MakeRotateZ(targetRoll);
	CalyxMath::Quaternion pitchQ		 = CalyxMath::Quaternion::MakeRotateX(targetPitch);
	CalyxMath::Quaternion targetRotation = CalyxMath::Quaternion::Multiply(rollQ, pitchQ);

	worldTransform_.rotation	   = CalyxMath::Quaternion::Slerp(worldTransform_.rotation, targetRotation, 0.15f);
	worldTransform_.rotationSource = RotationSource::Quaternion;

	// カメラ回転（Euler）
	CalyxMath::Vector3 currentRot = cam->GetRotate();
	currentRot.x				  = CalyxMath::Lerp(currentRot.x, targetPitch * 0.3f, 0.15f); // pitch
	currentRot.z				  = CalyxMath::Lerp(currentRot.z, targetRoll * 0.3f, 0.15f);  // roll
	cam->SetCamera(cam->GetTranslate(), currentRot);
}

///////////////////////////////////////////////////////////////////////////////////
//		レティクルの座標更新
///////////////////////////////////////////////////////////////////////////////////

void Player::MakeSerializableParam() {
	param_.LoadParams();

	moveSpeed_ = param_.moveSpeed;
}

CalyxEngine::ParamPath Player::GetParamPath() const {
	return {
		CalyxEngine::ParamDomain::Game,
		SceneObject::GetName()};
}

/////////////////////////////////////////////////////////////////////////////////////////
//		PlayerParam
/////////////////////////////////////////////////////////////////////////////////////////
Player::PlayerParam::PlayerParam() {
	AddField("moveSpeed", moveSpeed).Category("Basic").Range(0.0f, 50.0f);
	AddField("life", life).Category("Basic").Range(1, 100);

	AddField("collisionRadius", col.radius).Category("Collider").Range(0.1f, 10.0f);
	AddField("collisionOffset", col.offset).Category("Collider");

	AddField("hpGaugePos", hp.pos).Category("UI/HpGauge");
	AddField("hpGaugeSize", hp.size).Category("UI/HpGauge");

	AddField("reticleInitialZ", ret.initialZ).Category("UI/Reticle").Range(10.0f, 1000.0f);
	AddField("reticleMinSize", ret.minSize).Category("UI/Reticle").Range(1.0f, 512.0f);
	AddField("reticleMaxSize", ret.maxSize).Category("UI/Reticle").Range(1.0f, 512.0f);
	AddField("reticleRotSpeed", ret.rotSpeed).Category("UI/Reticle").Range(-1.0f, 1.0f);
	AddField("reticleUvRotSpeed", ret.uvRotSpeed).Category("UI/Reticle").Range(-5.0f, 5.0f);
}

CalyxEngine::ParamPath Player::PlayerParam::GetParamPath() const {
	return {CalyxEngine::ParamDomain::Game, "Player", "Actor/Player"};
}

void Player::HeaderGui() {
	if(ImGui::Button("Load All Sub-Systems")) {
		lockOn_->LoadConfig();
		dodgeSystem_->LoadConfig();
		danger_->LoadParam();
		damageHandler_->LoadParam();
	}
	ImGui::SameLine();
	if(ImGui::Button("Save All Sub-Systems")) {
		lockOn_->SaveConfig();
		dodgeSystem_->SaveConfig();
		danger_->SaveParam();
		damageHandler_->SaveParam();
	}
}

/* ======================================================================================
/*		accessor
/* ==================================================================================== */
void Player::SetParent(WorldTransform* parent) { worldTransform_.parent = parent; }

void Player::AttachEnemyList(const std::vector<std::shared_ptr<Enemy>>& list) const {
	if(lockOn_) {
		lockOn_->SetEnemyList(list);
	}
	if(reticle_) {
		reticle_->SetEnemyList(list);
	}
}

// std::vector<Sprite*> Player::GetAllSprites() const {
// }

const CalyxMath::Vector3 Player::GetCenterPos() const {
	const CalyxMath::Vector3 offset	  = {0.0f, 3.0f, 0.0f};
	CalyxMath::Vector3		 worldPos = CalyxMath::Vector3::Transform(offset, worldTransform_.matrix.world);
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

REGISTER_SCENE_OBJECT(Player)