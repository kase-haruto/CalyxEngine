#pragma once

#include "Engine/objects/Collider/Collider.h"

// c++
#include <list>
#include <vector>
#include <unordered_set>

/*-----------------------------------------------------------------------------------------
 * CollisionManager
 * - コリジョンマネージャークラス
 * - シーン内の全コライダーの登録・総当たり判定・衝突イベント発行を担当
 *---------------------------------------------------------------------------------------*/
class CollisionManager{
public:
	using CollisionShape = std::variant<Sphere, OBB>;
	//===================================================================*/
	//                   singleton
	//===================================================================*/
	static CollisionManager* GetInstance();
	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;

public:
	bool ShouldLogCollision(const Collider* a, const Collider* b);
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void UpdateCollisionAllCollider();			// すべてのコライダーを総当たりで判定

	Collider* FindColliderByName(const std::string& name);

	void Register(Collider* collider);		// コライダーリストに追加
	void Unregister(Collider* collider);	// コライダーリストから削除
	void DebugLog();
	void ClearColliders();

private:
	//===================================================================*/
	//                   private functions
	//===================================================================*/
	CollisionManager();
	~CollisionManager() = default;

	bool CheckCollisionPair(Collider* colliderA, Collider* colliderB);
	std::string MakeCollisionKey(Collider* colliderA, Collider* colliderB);

	void ComputeOBBAxes(const OBB& obb, CalyxMath::Vector3 outAxis[3]);
	float ProjectOBB(const OBB& obb, const CalyxMath::Vector3 obbAxes[3], const CalyxMath::Vector3& axisCandidate);
	bool OverlapOnAxis(
		const OBB& obbA, const CalyxMath::Vector3 aAxes[3],
		const OBB& obbB, const CalyxMath::Vector3 bAxes[3],
		const CalyxMath::Vector3& axisCandidate);

	/*----------------
	 各形状ごとの衝突
	----------------*/
	bool SphereToSphere(const Sphere& sphereA, const Sphere& sphereB);
	bool SphereToOBB(const Sphere& sphere, const OBB obb);
	bool OBBToOBB(const OBB& obbA, const OBB& obbB);


private:
	//===================================================================*/
	//                   private variable
	//===================================================================*/
	std::list<Collider*>colliders_;
	std::vector<std::string> collisionLogs_; // 衝突ログ

	std::unordered_set<std::string> currentCollisions_;  // 現在のフレームの衝突ペア
	std::unordered_set<std::string> previousCollisions_; // 前のフレームの衝突ペア
};

