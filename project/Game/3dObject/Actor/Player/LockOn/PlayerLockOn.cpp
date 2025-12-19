#include "PlayerLockOn.h"

// engine
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine\Foundation/Utility\Func\CxUtils.h>

// game
#include <Game/3dObject/Actor/Player/Player.h>

PlayerLockOn::PlayerLockOn()  = default;
PlayerLockOn::~PlayerLockOn() = default;

//////////////////////////////////////////////////////////////////////////////
//		初期化
//////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::Initialize(const PlayerActionContext& ctx) {
	ctx_ 				   = ctx;
	PrewarmMarkers(maxLockOn_);
}

///////////////////////////////////////////////////////////////////////////////
//		 更新
///////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::Update(float dt) {
	PurgeDeadLockedTargets();
	UpdateAutoLockOn(dt);

	// マーカー追従・演出
	for(size_t i = 0; i < lockOnSprites_.size(); ++i) {
		auto& enemy = lockedOnTargets_[i];
		if(!enemy) continue;

		Vector2 pos = Cx::Math::WorldToScreen(enemy->GetCenterPos());
		lockOnSprites_[i]->SetPosition(pos);

		float r = lockOnSprites_[i]->GetRotation() + 0.05f;
		lockOnSprites_[i]->SetRotation(r);

		float scale = 1.0f + 0.1f * std::sin(r * 4.0f);
		lockOnSprites_[i]->SetSize({64.0f * scale, 64.0f * scale});

		lockOnSprites_[i]->Update();
	}
}

///////////////////////////////////////////////////////////////////////////////
//		ロックオン要求
///////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::RequestLockOn() {
	if(lockedOnTargets_.size() >= maxLockOn_) return;

	// constexpr size_t kMaxLockOn = 4; //< 使っているなら削除 or 下の maxLockOn_ に置換
	auto* cam = CameraManager::GetMain3d();
	if(!cam) return;

	// 画面上のレティクル座標
	const Vector2 reticleScreen =
		Cx::Math::WorldToScreen(ctx_.getReticlePos());

	// ヒット判定
	for(const auto& enemy : targets_) {
		if(!enemy) continue;
		// ターゲットがいなかったらスキップ
		if(std::find(lockedOnTargets_.begin(), lockedOnTargets_.end(), enemy) != lockedOnTargets_.end()) continue;
		// カメラに映っていなかったらスキップ
		if(!cam->IsVisible(enemy->GetWorldAABB())) continue;

		Vector2 enemyScreen = Cx::Math::WorldToScreen(enemy->GetWorldPosition());
		if((enemyScreen - reticleScreen).Length() > lockOnRadiusPx_) continue;

		lockedOnTargets_.push_back(enemy);

		// マーカー生成
		auto marker = AcquireMarker();
		if(!marker) break;

		marker->SetPosition(enemyScreen);
		marker->SetSize({64.0f, 64.0f});
		marker->SetRotation(0.0f);
		marker->SetUvRotate(0.0f);
		lockOnSprites_.push_back(std::move(marker));

		if(lockedOnTargets_.size() >= maxLockOn_) break;
	}
}

///////////////////////////////////////////////////////////////////////////////
//		ロックオン解除要求
///////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::RequestLockOnClear() {
	for(auto& s : lockOnSprites_) {
		RecycleMarker(std::move(s));
	}
	lockOnSprites_.clear();
	lockedOnTargets_.clear();
}

//////////////////////////////////////////////////////////////////////////////
//		自動ロックオン更新
//////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::UpdateAutoLockOn(float dt) {
	lockOnRefreshTimer_ -= dt;
	// まだ間隔が来ていない
	if(lockOnRefreshTimer_ > 0.0f) return;
	lockOnRefreshTimer_ = lockOnRefreshInterval_;

	auto* cam = CameraManager::GetMain3d();
	if(!cam) return;

	const Vector2 reticleScreen =
		Cx::Math::WorldToScreen(ctx_.getReticlePos());

	for(size_t i = 0; i < lockedOnTargets_.size();) {
		auto& enemy	 = lockedOnTargets_[i];
		bool  remove = false;

		// 敵が死んでいたら解除
		if(!enemy || !enemy->GetIsAlive()) remove = true;
		// enemyがカメラから外れたら解除
		else if(!cam->IsVisible(enemy->GetWorldAABB()))
			remove = true;
		else {
			// レティクルとの距離が外れたら解除
			Vector2 enemyScreen = Cx::Math::WorldToScreen(enemy->GetWorldPosition());
			float	dist		= (enemyScreen - reticleScreen).Length();
			if(dist > lockOnReleaseRadiusPx_) remove = true;
		}

		// ロックオン解除処理
		if(remove) {
			RecycleMarker(std::move(lockOnSprites_[i]));
			lockOnSprites_.erase(lockOnSprites_.begin() + i);
			lockedOnTargets_.erase(lockedOnTargets_.begin() + i);
			continue;
		}
		++i;
	}

	// 新規取得
	if(lockedOnTargets_.size() < maxLockOn_) {
		for(const auto& e : targets_) {
			if(!e || !e->GetIsAlive()) continue;
			if(std::find(lockedOnTargets_.begin(), lockedOnTargets_.end(), e) != lockedOnTargets_.end()) continue;
			if(!cam->IsVisible(e->GetWorldAABB())) continue;

			// レティクルとの距離判定
			Vector2 s = Cx::Math::WorldToScreen(e->GetWorldPosition());
			float	d = (s - reticleScreen).Length();
			if(d > lockOnAcquireRadiusPx_) continue;

			// ヒット：登録 & マーカー生成
			auto marker = AcquireMarker();
			if(!marker) break;

			// 位置・サイズなどセット（Initializeは再度やらない）
			lockedOnTargets_.push_back(e);
			marker->SetPosition(s);
			marker->SetSize({64.0f, 64.0f});
			lockOnSprites_.push_back(std::move(marker));

			if(lockedOnTargets_.size() >= maxLockOn_) break;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//		死んだ敵のロックオン解除
/////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::PurgeDeadLockedTargets() {
	for(size_t i = 0; i < lockedOnTargets_.size();) {
		auto& e = lockedOnTargets_[i];
		if(!e || !e->GetIsAlive()) {
			RecycleMarker(std::move(lockOnSprites_[i]));
			lockOnSprites_.erase(lockOnSprites_.begin() + i);
			lockedOnTargets_.erase(lockedOnTargets_.begin() + i);
			continue;
		}
		++i;
	}
}

/////////////////////////////////////////////////////////////////////////////
//		取得
/////////////////////////////////////////////////////////////////////////////
std::unique_ptr<Sprite> PlayerLockOn::AcquireMarker() {
	if(!markerPool_.empty()) {
		auto s = std::move(markerPool_.back());
		markerPool_.pop_back();
		s->SetIsVisible(true);
		return s;
	}

	if(lockOnSprites_.size() + markerPool_.size() < maxLockOn_) {
		auto s = std::make_unique<Sprite>("Textures/lockOn.png");
		s->Initialize({0, 0}, {64, 64});
		s->SetAnchorPoint({0.5f, 0.5f});
		s->SetIsVisible(true);
		return s;
	}
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////
//		返却
/////////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::RecycleMarker(std::unique_ptr<Sprite> s) {
	if(!s) return;
	s->SetIsVisible(false);
	s->SetPosition({-10000.0f, -10000.0f});
	markerPool_.push_back(std::move(s));
}

///////////////////////////////////////////////////////////////////////////////
//		ロックオンマーカーの初期確保
///////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::PrewarmMarkers(size_t n) {
	markerPool_.reserve(n);
	for(size_t i = 0; i < n; ++i) {
		auto s = std::make_unique<Sprite>("Textures/lockOn.png");
		s->Initialize({-10000.0f, -10000.0f}, {64, 64});
		s->SetAnchorPoint({0.5f, 0.5f});
		s->SetIsVisible(false);
		markerPool_.push_back(std::move(s));
	}
}

//////////////////////////////////////////////////////////////////////////////
//		accessor
//////////////////////////////////////////////////////////////////////////////
void PlayerLockOn::SetEnemyList(const std::list<std::shared_ptr<Enemy>>& list) {
	targets_ = list;
}

const std::vector<std::shared_ptr<Enemy>>& PlayerLockOn::GetLockedTargets() const {
	return lockedOnTargets_;
}

std::vector<Sprite*> PlayerLockOn::GetSprites() const {
	std::vector<Sprite*> out;
	for(auto& s : lockOnSprites_) out.push_back(s.get());
	return out;
}