#include "EnemyEngagementService.h"

// engine
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>

// game
#include <Game/3dObject/Actor/Enemy/Directory/EnemyDirectory.h>
#include <Game/3dObject/Actor/Enemy/Enemy.h>
#include <Game/3dObject/Actor/Player/Player.h>

// helpers
#include <Engine/Application/Gameplay/Visibility/GameplayVisibility.h>
#include <Engine/Application/Gameplay/Combat/LineOfSight.h>

void EnemyEngagementService::OnSceneLoaded(SceneContext& ctx) {
	RefreshRefs(ctx);
	exposeSec_.clear();
}

void EnemyEngagementService::RefreshRefs(SceneContext& ctx) {
	camera_ = CameraManager::GetMain3d();
	if (wPlayer_.expired())  wPlayer_ = ctx.FindFirst<Player>();
	if (wDir_.expired())     wDir_ = ctx.FindFirst<EnemyDirectory>(); // 注入が無い場合のみ探す
}

bool EnemyEngagementService::HasLineOfSightImpl(const Enemy& e, const Player& p) const {
	if (!useLOS_) return true;
	if (!isPlayer_) return true;
	if (!buildCandidates_) return true;

	const CalyxMath::Vector3 from = e.GetCenterPos();
	const CalyxMath::Vector3 to = p.GetCenterPos();

	std::vector<SceneObject*> candidates;
	buildCandidates_(candidates, /*ignore*/ static_cast<const SceneObject*>(&e));

	return HasLineOfSight(from, to, candidates, isPlayer_);
}

void EnemyEngagementService::Update(float dt) {
	// 参照が切れていたら再解決
	if (auto* ctx = SceneContext::Current()) {
		if (!camera_) camera_ = CameraManager::GetMain3d();
		if (wPlayer_.expired() || wDir_.expired()) {
			RefreshRefs(*ctx);
		}
	}

	auto* ctx = SceneContext::Current();
	auto ply = wPlayer_.lock();

	// ---- 敵リストの取得（ディレクトリ or フォールバック）----
	std::vector<std::shared_ptr<Enemy>> enemies; enemies.reserve(64);

	//
	if (ctx && ctx->GetObjectLibrary()) {
		auto all = ctx->GetObjectLibrary()->FindByType<Enemy>();
		for (auto& sp : all) {
			if (sp && sp->GetIsAlive()) enemies.emplace_back(sp);
		}
	}

	if (enemies.empty()) return;

	// ---- 各敵を評価 ----
	for (auto& sp : enemies) {
		if (!sp) continue;
		Enemy& e = *sp;

		bool onScreen = true;
		if (camera_) {
			const AABB worldAabb = e.GetWorldAABB();
			onScreen = GameplayVisibility::IsAabbOnScreenNdc(camera_, worldAabb, ndcPad_);
		}

		// 露出時間の積算
		float& t = exposeSec_[&e];
		t = onScreen ? (t + dt) : 0.0f;
		const bool exposedEnough = (t >= minExposeSec_);

		//  射程
		bool inRange = true;
		if (ply) {
			inRange = GameplayVisibility::InEngageRangeSq(
				e.GetWorldPosition(), ply->GetWorldPosition(), maxEngageDist_);
		}

		bool los = true;
		if (ply) {
			los = HasLineOfSightImpl(e, *ply);
		}

		const bool allow = exposedEnough && inRange && los;
		e.SetGameplayEngaged(allow);
	}
}
