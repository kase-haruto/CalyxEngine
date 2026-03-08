#include "Reticle.h"
#include <Game/3dObject/Actor/Enemy/Enemy.h>

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
	reticleSprite_->SetPosition({400.0f, 300.0f});
	reticleSprite_->GetSprite()->SetAnchorPoint(param_.spriteParam_.anchorPoint);
	reticleSprite_->SetScale(param_.spriteParam_.scale);
	transform_.Initialize();
	transform_.translation.z = param_.posFar;
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
	if(reticleSprite_) {
		reticleSprite_->Update(dt);
	}
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

void Reticle::SetEnemyList(const std::vector<std::shared_ptr<Enemy>>& list) {
	targets_.clear();
	targets_.reserve(list.size());
	for(auto& e : list) {
		targets_.push_back(e);
	}
}

void Reticle::SetLockedEnemyList(const std::vector<std::shared_ptr<Enemy>>& list) {
	lockedTargets_.clear();
	lockedTargets_.reserve(list.size());
	for(auto& e : list) {
		lockedTargets_.push_back(e);
	}
}

//////////////////////////////////////////////////////////////////////////////
//	レティクル座標を取得
//////////////////////////////////////////////////////////////////////////////
const CalyxMath::Vector2& Reticle::GetPosition() const { return reticleSprite_->GetPosition(); }
CalyxMath::Vector3		  Reticle::GetPosition3D() const { return transform_.GetWorldPosition(); }

//////////////////////////////////////////////////////////////////////////////
//	レティクル座標を取得
//////////////////////////////////////////////////////////////////////////////
void Reticle::ApplyMove(float dt) {
	auto* input = CalyxFoundation::Input::GetInstance();

	// 右スティック入力（-1 ～ 1）
	CalyxMath::Vector2 rs = input->GetRightStick();

	// 微少入力の揺れ対策
	const float stickDeadZone = 0.15f;
	const float mag			  = std::sqrt(rs.x * rs.x + rs.y * rs.y);

	// 無入力時は dir をゼロにしてアシストは続行する（早期リターンしない）
	CalyxMath::Vector2 dir = {0.0f, 0.0f};
	if(mag >= stickDeadZone) {
		// deadZone を抜けた部分を 0..1 に再マップ（滑らかに動く）
		const float t	  = (mag - stickDeadZone) / (1.0f - stickDeadZone);
		const float scale = std::clamp(t, 0.0f, 1.0f) / mag;
		dir				  = {rs.x * scale, rs.y * scale};
	}

	// speed（パラメータ）と dt の適用
	CalyxMath::Vector2 offset = dir * param_.speed * dt;
	transform_.translation.x += 10 * offset.x;
	transform_.translation.y += 10 * offset.y;
	// 常に Z を固定（初期化や補間で変わるのを防ぐ）
	transform_.translation.z = param_.posFar;

	// --- 画面内クランプ ---
	if(Camera3d* cam = CameraManager::GetMain3d()) {
		// レティクルはカメラの子なので、translation は既にカメラローカル空間
		// その Z 距離での表示可能範囲を計算
		float halfHeight = std::tan(cam->GetFovY() * 0.5f) * transform_.translation.z;
		float halfWidth	 = halfHeight * cam->GetAspectRatio();

		// クランプ
		float margin			 = 0.95f; // レティクルは端まで行けるように少し広め
		transform_.translation.x = std::clamp(transform_.translation.x, -halfWidth * margin, halfWidth * margin);
		transform_.translation.y = std::clamp(transform_.translation.y, -halfHeight * margin, halfHeight * margin);
	}

	// ── アシスト処理 ─────────────────────────
	if(targets_.empty() || !transform_.parent) return;

	std::shared_ptr<Enemy> closestEnemy	 = nullptr;
	float				   minSqDist	 = param_.assistRadiusPx * param_.assistRadiusPx;
	CalyxMath::Vector2	   reticleScreen = CalyxMath::WorldToScreen(transform_.GetWorldPosition());

	// 無効なターゲットを削除
	auto rmIt = std::remove_if(targets_.begin(), targets_.end(), [](const std::weak_ptr<Enemy>& wp) {
		auto sp = wp.lock();
		return !sp || !sp->GetIsAlive();
	});
	targets_.erase(rmIt, targets_.end());

	for(auto it = targets_.begin(); it != targets_.end(); ++it) {
		auto enemy = it->lock();
		// ロックオン済みの敵はアシスト対象から除外
		bool isLocked = false;
		for(const auto& locked : lockedTargets_) {
			if(locked.lock() == enemy) {
				isLocked = true;
				break;
			}
		}
		if(isLocked) continue;

		CalyxMath::Vector2 enemyScreen = CalyxMath::WorldToScreen(enemy->GetCenterPos());
		float			   sqDist	   = (enemyScreen - reticleScreen).LengthSquared();
		if(sqDist < minSqDist) {
			minSqDist	 = sqDist;
			closestEnemy = enemy;
		}
	}

	if(closestEnemy) {
		// カメラ空間での敵の座標を取得し、レティクル平面(Z=posFar)に投影
		CalyxMath::Vector3	 enemyWorld = closestEnemy->GetCenterPos();
		CalyxMath::Matrix4x4 invCam		= CalyxMath::Matrix4x4::Inverse(transform_.parent->matrix.world);
		CalyxMath::Vector3	 enemyLocal = CalyxMath::Vector3::Transform(enemyWorld, invCam);

		if(enemyLocal.z > 0.001f) {
			float projScale = param_.posFar / enemyLocal.z;
			float targetX	= enemyLocal.x * projScale;
			float targetY	= enemyLocal.y * projScale;

			float dist	 = std::sqrt(minSqDist);
			float weight = 1.0f - std::clamp(dist / param_.assistRadiusPx, 0.0f, 1.0f);

			CalyxMath::Vector2 enemyScreen = CalyxMath::WorldToScreen(enemyWorld);
			CalyxMath::Vector2 toEnemyDir  = (enemyScreen - reticleScreen).Normalize();
			float			   dotInput	   = dir.x * toEnemyDir.x + dir.y * toEnemyDir.y;
			if(dotInput < 0.0f) {
				weight *= 0.5f;
			}

			float interpT			 = 1.0f - std::pow(1.0f - (param_.assistStrength * weight), dt * 60.0f);
			transform_.translation.x = CalyxMath::Lerp(transform_.translation.x, targetX, interpT);
			transform_.translation.y = CalyxMath::Lerp(transform_.translation.y, targetY, interpT);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////
//	パラメータ
//////////////////////////////////////////////////////////////////////////////
Reticle::ReticleParam::ReticleParam() {
	AddField("posFar", posFar).Category("reticle").Range(100.0f, 5000.0f);
	AddField("speed", speed).Category("reticle").Range(5.0f, 30.0f);
	AddField("assistRadiusPx", assistRadiusPx).Category("reticle").Range(0.0f, 500.0f);
	AddField("assistStrength", assistStrength).Category("reticle").Range(0.0f, 1.0f);
	AddField("anchorPoint", spriteParam_.anchorPoint).Category("reticle");
	AddField("scale", spriteParam_.scale).Category("reticle");
}

//////////////////////////////////////////////////////////////////////////////
//	パラメータのパス
//////////////////////////////////////////////////////////////////////////////
CalyxEngine::ParamPath Reticle::ReticleParam::GetParamPath() const { return {CalyxEngine::ParamDomain::Game, "Reticle", "Player/Reticle"}; }