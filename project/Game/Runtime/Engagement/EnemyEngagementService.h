#pragma once
#include <memory>
#include <unordered_map>
#include <functional>
#include <vector>

class SceneContext;
class Camera3d;
class Player;
class Enemy;
class SceneObject;
class EnemyDirectory;

class EnemyEngagementService {
public:
	void OnSceneLoaded(SceneContext& ctx);
	void Update(float dt);

	// パラメータ
	void SetOnScreenPad(float ndcPad) { ndcPad_ = ndcPad; }
	void SetMinExposeTime(float seconds) { minExposeSec_ = seconds; }
	void SetMaxEngageDistance(float dist) { maxEngageDist_ = dist; }
	void EnableLineOfSight(bool enable) { useLOS_ = enable; }

	// LOS 用コールバック
	using IsPlayerFn = std::function<bool(void*)>;
	void SetLineOfSightPredicate(IsPlayerFn fn) { isPlayer_ = std::move(fn); }

	using BuildRaycastCandidatesFn = std::function<void(std::vector<SceneObject*>& out,
														const SceneObject* ignore)>;
	void SetRaycastCandidatesProvider(BuildRaycastCandidatesFn fn) { buildCandidates_ = std::move(fn); }
	void SetDirectory(std::shared_ptr<EnemyDirectory> dir) { wDir_ = std::move(dir); }
private:
	void RefreshRefs(SceneContext& ctx);
	bool HasLineOfSightImpl(const Enemy& e, const Player& p) const;

private:
	Camera3d* camera_ = nullptr;
	std::weak_ptr<Player> wPlayer_;
	std::weak_ptr<EnemyDirectory> wDir_;

	float ndcPad_ = 0.05f;			// 5% 内側
	float minExposeSec_ = 0.20f;	// 0.2秒以上露出
	float maxEngageDist_ = 120.0f;	// 射程
	bool  useLOS_ = false;

	IsPlayerFn isPlayer_;						// LOS のプレイヤー判定
	BuildRaycastCandidatesFn buildCandidates_;	// レイキャスト候補生成

	std::unordered_map<Enemy*, float> exposeSec_;
};
