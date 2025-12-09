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

inline void SetWorldPosKeepRotScale(WorldTransform& wt,const Vector3& worldPos) {
	// 親が何段でもOK：親の world は祖先込み合成
	if(wt.parent) {
		wt.parent->Update(); // 親の world を最新に
		const Matrix4x4 invParent = Matrix4x4::Inverse(wt.parent->matrix.world);
		// 位置のみをローカルへ戻す（回転・スケールはローカル既存値を維持）
		wt.translation = Vector3::Transform(worldPos,invParent);
	} else { wt.translation = worldPos; }
}

// world を「画面内の矩形（px余白あり）」に収めたワールド座標へ
inline Vector3 ClampWorldByScreenBox(const Vector3& world,
									 float          marginXpx,float marginYpx) {
	auto* cam = CameraManager::GetMain3d();
	if(!cam) return world;

	// スクリーン座標と NDC z を取得
	const Matrix4x4& VP   = cam->GetViewProjectionMatrix();
	const Vector4    clip = Vector4::Transform(Vector4(world,1.0f),VP);

	// 背面や極端ケースは触らない
	constexpr float kEps = 1e-6f;
	if(clip.w <= kEps) return world;

	const Vector3 ndc = {clip.x / clip.w,clip.y / clip.w,clip.z / clip.w};
	Vector2       scr = Cx::Math::WorldToScreen(world);

	// 画面サイズと余白でクランプ（float化しておく）
	constexpr float W = static_cast<float>(kGameWidth);
	constexpr float H = static_cast<float>(kGameHeight);

	const float minX = (std::max)(0.0f,marginXpx);
	const float maxX = (std::max)(minX,W - marginXpx);
	const float minY = (std::max)(0.0f,marginYpx);
	const float maxY = (std::max)(minY,H - marginYpx);

	const Vector2 clamped = {
		std::clamp(scr.x,minX,maxX),
		std::clamp(scr.y,minY,maxY)};

	// 変化なしなら元のワールドを返す
	if(clamped.x == scr.x && clamped.y == scr.y) return world;

	return Cx::Math::ScreenToWorld(clamped,ndc.z);
}

// WorldTransform を画面内にクランプ（ローカル translation へ反映まで）
inline void ClampWorldTransformInView(WorldTransform& wt,
									  float           marginXpx,float marginYpx) {
	// 自身・親の world を最新化
	wt.Update();

	const Vector3 nowW = wt.GetWorldPosition();
	const Vector3 clW  = ClampWorldByScreenBox(nowW,marginXpx,marginYpx);

	// 親が何段でもローカルへ戻して translation を更新
	SetWorldPosKeepRotScale(wt,clW);
}

} // namespace

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
Player::Player(const std::string&         modelName,
			   std::optional<std::string> objectName)
	: Actor::Actor(modelName,objectName) {
	worldTransform_.translation = {0.0f,0.0f,8.0f};
	worldTransform_.scale       = {1.5f,1.5f,1.5f};
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
	reticleTransform_.parent      = &CameraManager::GetMain3d()->GetWorldTransform();
	reticleTransform_.translation = Vector3(0.0f,0.0f,100.0f);

	// コライダー初期化
	BaseGameObject::InitializeCollider(ColliderKind::Sphere);
	collider_->SetType(ColliderType::Type_Player);
	collider_->SetTargetType(ColliderType::Type_EnemyAttack);
	collider_->SetOwner(this);
	collider_->SetIsDrawCollider(true);
	if(auto* radius = dynamic_cast<SphereCollider*>(collider_.get())) { radius->SetRadius(1.5f); }
	collider_->SetOffset({0.0f,-2.0f,0.0f});
	collider_->SetCollisionEnabled(true);

	// life関連初期化
	Actor::SetLife(30);

	// ライフゲージの初期化
	hpGauge_ = std::make_unique<HpGauge>(static_cast<float>(life_));
	// 左下にライフゲージを設定
	Vector2 lifeGaugePos = {100.0f,630.0f};
	hpGauge_->Initialize(lifeGaugePos,Vector2(360.0f,32.0f));
	hpGauge_->SetAncorPoint({0.0f,0.5f}); // 左中央

	// spriteの初期化
	size_t spriteCount = reticleSprites_.size();
	for(size_t i = 0; i < spriteCount; ++i) {
		reticleSprites_[i] = std::make_unique<Sprite>("Textures/reticle.png");

		float   t    = static_cast<float>(i) / (spriteCount - 1);
		float   size = std::lerp(128.0f,16.0f,t);
		Vector2 spriteSize(size,size);

		Vector2 initPos = kGameSize * 0.5f;
		reticleSprites_[i]->Initialize(initPos,spriteSize);
		reticleSprites_[i]->SetAnchorPoint(Vector2(0.5f,0.5f));
	}

	// ---- 回避コンポーネント ----
	if(!dodgeSystem_) {
		PlayerDodgeConfig cfg;
		cfg.useCustomCurve = true;
		cfg.lateralScale   = 0.0f;
		cfg.backwardScale  = 0.70f;
		cfg.spinTurns      = 1.0f;

		dodgeSystem_ = std::make_unique<PlayerDodgeSystem>();
		dodgeSystem_->Initialize(this,cfg);
	}

	// 回避モーション
	if(!dodgeMotion_) {
		dodgeMotion_ = std::make_unique<PlayerDodgeMotion>();
		dodgeMotion_->Initialize(this,dodgeSystem_.get());
	}

	// 危険察知
	if(!danger_) {
		danger_ = std::make_unique<PlayerDangerSense>();
		danger_->Initialize(this,dodgeSystem_.get(),{}); // UIやmarginは後で調整可
	}

	// DamageHandler
	if(!damageHandler_) {
		damageHandler_ = std::make_unique<PlayerDamageHandler>();
		damageHandler_->Initialize(this);
	}
	dodgeSystem_->SetOnRequestInvincible(
		[this](float sec) { if(damageHandler_) { damageHandler_->RequestInvincible(sec); } }
		);

	PrewarmLockMarkers(maxLockOn_);

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
	if(inputHandler_) { inputHandler_->Update(*this,dt); }
	if(dodgeSystem_) { dodgeSystem_->Update(dt); }
	if(dodgeMotion_) { dodgeMotion_->Update(dt); }
	if(damageHandler_) { damageHandler_->Update(dt); }

	moveCtrler_.Apply(worldTransform_);

	if(shootingController_) { shootingController_->Update(dt); }

	if(danger_) danger_->Update(dt);

	if(hpGauge_) {
		hpGauge_->Update(dt);
		hpGauge_->SetHp(static_cast<float>(life_));
	}
	reticleTransform_.Update();

	// 自動ロックオン
	PurgeDeadLockedTargets();
	UpdateAutoLockOn(dt);

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
		float rotateSpeed   = (i % 2 == 0) ? 0.02f : -0.02f;
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
		lockOnSprites_[i]->SetSize({64.0f * scale,64.0f * scale});

		lockOnSprites_[i]->Update();
		++i;
	}

	if(life_ <= 0) { isAlive_ = false; }
}

/////////////////////////////////////////////////////////////////////////////////////////
//		描画
/////////////////////////////////////////////////////////////////////////////////////////
void Player::Draw([[maybe_unused]] ID3D12GraphicsCommandList* cmdList) {}

/////////////////////////////////////////////////////////////////////////////////////////
//		imgui
/////////////////////////////////////////////////////////////////////////////////////////
void Player::DerivativeGui() {
	if(hpGauge_) { hpGauge_->ShowGui(); }

	ImGui::DragFloat("moveSpeed",&moveSpeed_,0.01f,0.0f,10.0f);
	ImGui::DragFloat("lockOnRadius(px)",&lockOnRadiusPx_,1.0f,10.0f,400.0f);
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
void Player::RequestShoot() {
	Vector3 playerPos  = worldTransform_.GetWorldPosition();
	Vector3 reticlePos = reticleTransform_.GetWorldPosition();
	Vector3 dir        = reticlePos - playerPos;

	if(dir.Length() > 0.001f) { dir = dir.Normalize(); } else { dir = Vector3(0.0f,0.0f,1.0f); }

	if(!shootingController_) return;

	// ロックオンしていればホーミングする弾を撃つ
	shootingController_->SetTargets(lockedOnTargets_);
	shootingController_->SetMode(lockedOnTargets_.empty()
									 ? PlayerShoot::BulletMode::Straight
									 : PlayerShoot::BulletMode::Homing);

	// 発射されていたらエフェクトを再生
	if(bool isFired = shootingController_->RequestShoot(playerPos,dir)) {
		// 発射エフェクト
		shootFx_->PlayAll();
	}

	RequestLockOnTargetClear();
}

///////////////////////////////////////////////////////////////////////////////////
//		ロックオン処理
///////////////////////////////////////////////////////////////////////////////////
void Player::RequestLockOn() {
	// constexpr size_t kMaxLockOn = 4; ← 使っているなら削除 or 下の maxLockOn_ に置換
	if(lockedOnTargets_.size() >= maxLockOn_) return;

	Camera3d* camera = CameraManager::GetMain3d();
	if(!camera) return;

	const Vector2 reticleScreen = Cx::Math::WorldToScreen(reticleTransform_.GetWorldPosition());

	const float radius = lockOnRadiusPx_;

	for(const auto& enemy : targets_) {
		if(!enemy) continue;
		if(std::find(lockedOnTargets_.begin(),lockedOnTargets_.end(),enemy) != lockedOnTargets_.end()) continue;
		if(!camera->IsVisible(enemy->GetWorldAABB())) continue;

		Vector2 enemyScreen = Cx::Math::WorldToScreen(enemy->GetWorldPosition());
		if((enemyScreen - reticleScreen).Length() > radius) continue;

		// ヒット：登録 & マーカー生成
		lockedOnTargets_.push_back(enemy);

		auto marker = AcquireMarker();
		if(!marker) break;
		marker->SetPosition(enemyScreen);
		marker->SetSize({64.0f,64.0f});
		marker->SetRotation(0.0f);
		marker->SetUvRotate(0.0f);
		lockOnSprites_.push_back(std::move(marker));

		if(lockedOnTargets_.size() >= maxLockOn_) break;
	}
}

void Player::AttachDangerSenseSource(EnemyDirectory* dir) { if(danger_) danger_->SetEnemyDirectory(dir); }

void Player::RequestLockOnTargetClear() {
	// 再利用
	for(size_t i = 0; i < lockOnSprites_.size(); ++i) { RecycleMarker(std::move(lockOnSprites_[i])); }
	lockOnSprites_.clear();
	lockedOnTargets_.clear();
}

void Player::Start() {
	if(!inputHandler_) inputHandler_ = std::make_unique<PlayerInputHandler>();

	if(!shootingController_) {
		auto bullets        = SceneAPI::Instantiate<PlayerBulletContainer>("playerBulletController");
		shootingController_ = std::make_unique<PlayerShootingController>(bullets.get());
	}
}

void Player::RequestDodge() { if(dodgeSystem_) { dodgeSystem_->RequestDodge(); } }

///////////////////////////////////////////////////////////////////////////////////
//		衝突
///////////////////////////////////////////////////////////////////////////////////
void Player::OnCollisionEnter(Collider* other) {

	// イベントの場合スキップ
	if(other->GetType() == ColliderType::Type_EventObject) return;

	if(damageHandler_) { damageHandler_->OnHit(other); }
}

///////////////////////////////////////////////////////////////////////////////////
//		playerの傾き
///////////////////////////////////////////////////////////////////////////////////
void Player::UpdateTilt(const Vector3& inputVector) {
	Camera3d* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 閾値以下で戻す
	if(inputVector.Length() <= 0.01f) {
		Quaternion identity            = Quaternion::MakeIdentity();
		worldTransform_.rotation       = Quaternion::Slerp(worldTransform_.rotation,identity,0.1f);
		worldTransform_.rotationSource = RotationSource::Quaternion;

		// カメラ傾きを戻す（オイラー角）
		Vector3 currentRot = cam->GetRotate();
		currentRot.x       = Cx::Math::Lerp(currentRot.x,0.0f,0.1f); // pitch
		currentRot.z       = Cx::Math::Lerp(currentRot.z,0.0f,0.1f); // roll
		cam->SetCamera(cam->GetTranslate(),currentRot);
		return;
	}

	Vector3 dir = inputVector.Normalize();

	const float maxRoll  = 0.3f;
	const float maxPitch = 0.3f;

	float targetRoll  = -dir.x * maxRoll;
	float targetPitch = -dir.y * maxPitch;

	// プレイヤー回転（Quaternion）
	Quaternion rollQ          = Quaternion::MakeRotateZ(targetRoll);
	Quaternion pitchQ         = Quaternion::MakeRotateX(targetPitch);
	Quaternion targetRotation = Quaternion::Multiply(rollQ,pitchQ);

	worldTransform_.rotation       = Quaternion::Slerp(worldTransform_.rotation,targetRotation,0.15f);
	worldTransform_.rotationSource = RotationSource::Quaternion;

	// カメラ回転（Euler）
	Vector3 currentRot = cam->GetRotate();
	currentRot.x       = Cx::Math::Lerp(currentRot.x,targetPitch * 0.3f,0.15f); // pitch
	currentRot.z       = Cx::Math::Lerp(currentRot.z,targetRoll * 0.3f,0.15f);  // roll
	cam->SetCamera(cam->GetTranslate(),currentRot);
}

///////////////////////////////////////////////////////////////////////////////////
//		autoロックオン
///////////////////////////////////////////////////////////////////////////////////
void Player::UpdateAutoLockOn(float dt) {

	lockOnRefreshTimer_ -= dt;
	if(lockOnRefreshTimer_ > 0.0f) return;
	lockOnRefreshTimer_ = lockOnRefreshInterval_;

	auto* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 画面上のレティクル座標
	const Vector2 reticleScreen = Cx::Math::WorldToScreen(reticleTransform_.GetWorldPosition());

	// 既存ロックの維持/解除判定（解除は release 半径）
	for(size_t i = 0; i < lockedOnTargets_.size();) {
		auto& enemy  = lockedOnTargets_[i];
		bool  remove = false;

		// 敵が死んでいたらremove
		if(!enemy || !enemy->GetIsAlive()) { remove = true; }
		// enemyがカメラから外れたらremove
		else if(!cam->IsVisible(enemy->GetWorldAABB())) { remove = true; }
		// レティクルとの距離が外れたらremove
		else {
			Vector2 enemyScreen = Cx::Math::WorldToScreen(enemy->GetWorldPosition());
			float   dist        = (enemyScreen - reticleScreen).Length();
			if(dist > lockOnReleaseRadiusPx_) { remove = true; }
		}

		if(remove) {
			// ロックオン解除処理
			RecycleMarker(std::move(lockOnSprites_[i]));
			lockOnSprites_.erase(lockOnSprites_.begin() + i);
			lockedOnTargets_.erase(lockedOnTargets_.begin() + i);
			continue;
		}
		++i;
	}

	// 空きがあれば新規取得（acquire 半径以内 & 近い順）
	if(lockedOnTargets_.size() < maxLockOn_) {
		// 候補収集
		struct Cand {
			std::shared_ptr<Enemy> e;
			float                  d;
			Vector2                s;
		};
		std::vector<Cand> cands;
		cands.reserve(targets_.size());

		for(const auto& e : targets_) {
			if(!e || !e->GetIsAlive()) continue;
			if(std::find(lockedOnTargets_.begin(),lockedOnTargets_.end(),e) != lockedOnTargets_.end()) continue;
			if(!cam->IsVisible(e->GetWorldAABB())) continue;

			Vector2 s = Cx::Math::WorldToScreen(e->GetWorldPosition());
			float   d = (s - reticleScreen).Length();
			if(d <= lockOnAcquireRadiusPx_) { cands.push_back({e,d,s}); }
		}

		// 近い順
		std::sort(cands.begin(),cands.end(),[](const Cand& a,const Cand& b) { return a.d < b.d; });

		for(const auto& c : cands) {
			if(lockedOnTargets_.size() >= maxLockOn_) break;
			auto marker = AcquireMarker();
			if(!marker) break;                // 上限
			if(c.e->GetLife() <= 0) continue; // 死んでたらスキップ
			lockedOnTargets_.push_back(c.e);

			// 位置・サイズなどセット（Initializeは再度やらない）
			marker->SetPosition(c.s);
			marker->SetSize({64.0f,64.0f});
			marker->SetRotation(0.0f);
			marker->SetUvRotate(0.0f);

			lockOnSprites_.push_back(std::move(marker));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////
//		取得
///////////////////////////////////////////////////////////////////////////////////
std::unique_ptr<Sprite> Player::AcquireMarker() {
	if(!markerPool_.empty()) {
		auto s = std::move(markerPool_.back());
		markerPool_.pop_back();
		s->SetIsVisible(true);
		return s;
	}
	// まだ上限に達していないなら新規作成を許可（保険）
	if(lockOnSprites_.size() + markerPool_.size() < maxLockOn_) {
		auto s = std::make_unique<Sprite>("Textures/lockOn.png");
		s->Initialize({0.0f,0.0f},{64.0f,64.0f});
		s->SetAnchorPoint({0.5f,0.5f});
		s->SetIsVisible(true);
		return s;
	}
	return nullptr; // これ以上は持たない
}

///////////////////////////////////////////////////////////////////////////////////
//		返却
///////////////////////////////////////////////////////////////////////////////////
void Player::RecycleMarker(std::unique_ptr<Sprite> s) {
	if(!s) return;
	s->SetIsVisible(false);
	s->SetPosition({-10000.0f,-10000.0f}); // 画面外へ
	markerPool_.push_back(std::move(s));
}

///////////////////////////////////////////////////////////////////////////////////
//		初期確保
///////////////////////////////////////////////////////////////////////////////////
void Player::PrewarmLockMarkers(size_t n) {
	markerPool_.reserve(n);
	for(size_t i = 0; i < n; ++i) {
		auto s = std::make_unique<Sprite>("Textures/lockOn.png");
		// 一度だけGPUリソースを確保
		s->Initialize({-10000.0f,-10000.0f},{64.0f,64.0f});
		s->SetAnchorPoint({0.5f,0.5f});
		s->SetIsVisible(false);
		markerPool_.push_back(std::move(s));
	}
}

///////////////////////////////////////////////////////////////////////////////////
//		死んだ敵のロックオンを外す
///////////////////////////////////////////////////////////////////////////////////
void Player::PurgeDeadLockedTargets() {
	for(size_t i = 0; i < lockedOnTargets_.size();) {
		auto& e = lockedOnTargets_[i];
		// 敵が死んでたらロックオン解除
		if(!e || !e->GetIsAlive()) {
			RecycleMarker(std::move(lockOnSprites_[i]));
			lockOnSprites_.erase(lockOnSprites_.begin() + i);
			lockedOnTargets_.erase(lockedOnTargets_.begin() + i);
			continue;
		}
		++i;
	}
}

///////////////////////////////////////////////////////////////////////////////////
//		レティクルの座標更新
///////////////////////////////////////////////////////////////////////////////////
void Player::UpdateReticlePosition() {
	constexpr float moveSpeed        = 6.0f;
	constexpr float stickSensitivity = 300.0f; // スティック感度を大きめに
	float           dt               = ClockManager::GetInstance()->GetDeltaTime();

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
	if(clampReticleInView_) { ClampWorldTransformInView(reticleTransform_,clampMarginXpx_,clampMarginYpx_); }
}

/* ======================================================================================
/*		accessor
/* ==================================================================================== */
void Player::SetParent(WorldTransform* parent) { worldTransform_.parent = parent; }

std::vector<Sprite*> Player::GetAllSprites() {
	std::vector<Sprite*> sprites;
	for(auto& s : reticleSprites_) sprites.push_back(s.get());
	// for(auto& s : lifeSprite_) sprites.push_back(s.get());
	for(auto& s : lockOnSprites_) sprites.push_back(s.get());

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
	const Vector3 offset   = {0.0f,3.0f,0.0f};
	Vector3       worldPos = Vector3::Transform(offset,worldTransform_.matrix.world);
	return worldPos;
}

std::optional<float> Player::GetShootCooldown() {
	if(shootingController_) { return shootingController_->GetCooldown(); }

	return std::nullopt;
}

std::optional<const float> Player::GetMaxShootInterval() const {
	if(shootingController_) { return shootingController_->GetInterval(); }

	return std::nullopt;
}

void Player::SetShootingController(std::unique_ptr<PlayerShootingController> sc) { shootingController_ = std::move(sc); }

void Player::SetInputHandler(std::unique_ptr<PlayerInputHandler> ih) { inputHandler_ = std::move(ih); }

REGISTER_SCENE_OBJECT(Player)