#pragma once

#include "Engine/objects/Collider/Collider.h"

// c++
#include <list>
#include <unordered_set>
#include <vector>

/*-----------------------------------------------------------------------------------------
 * CollisionManager
 * - コリジョンマネージャークラス
 * - シーン内の全コライダーの登録・総当たり判定・衝突イベント発行を担当
 *---------------------------------------------------------------------------------------*/
/**
 * @brief CollisionManagerの機能を提供するクラスです。
 */
class CollisionManager {
public:
	//===================================================================*/
	//                   singleton
	//===================================================================*/
	/**
	 * \brief 衝突管理の共有インスタンスを取得する
	 * \return 関数ローカルstaticで所有する共有インスタンスへのポインタ
	 */
	static CollisionManager* GetInstance();
	/** \brief CollisionManagerのコピー構築を禁止する */
	CollisionManager(const CollisionManager&)			 = delete;
	/** \brief CollisionManagerのコピー代入を禁止する \return 代入結果は生成されない */
	CollisionManager& operator=(const CollisionManager&) = delete;

public:
	bool ShouldLogCollision(const Collider* a, const Collider* b);
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	void UpdateCollisionAllCollider(); // すべてのコライダーを総当たりで判定

	void Register(Collider* collider);	 // コライダーリストに追加
	void Unregister(Collider* collider); // コライダーリストから削除
	void DebugLog();
	void ClearColliders();
	std::vector<Collider*> GetCollidersSnapshot() const;

	/*-----------------------------------------------------------------------------------------
	 * CollisionPair
	 * - 1フレームで接触している2個のColliderを順序非依存で保持するデータ構造
	 * - Colliderの所有権は管理しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief CollisionPairに関するデータを保持する構造体です。
	 */
	struct CollisionPair {
		Collider* a; //< 所有権を持たない衝突ペアの一方
		Collider* b; //< 所有権を持たない衝突ペアの他方

		/** \brief Colliderの並び順を無視してペアが等しいか判定する \param other 比較対象のペア \return 同じ2個のColliderを参照する場合はtrue */
		bool operator==(const CollisionPair& other) const {
			return (a == other.a && b == other.b) || (a == other.b && b == other.a);
		}
	};

	/*-----------------------------------------------------------------------------------------
	 * CollisionPairHash
	 * - CollisionPairを順序非依存コンテナで使用するためのハッシュ関数オブジェクト
	 *---------------------------------------------------------------------------------------*/
	struct CollisionPairHash {
		/** \brief 衝突ペアのハッシュ値を計算する \param pair 計算対象のペア \return 2個のColliderアドレスから生成したハッシュ値 */
		size_t operator()(const CollisionPair& pair) const {
			auto h1 = std::hash<const Collider*>{}(pair.a);
			auto h2 = std::hash<const Collider*>{}(pair.b);
			return h1 ^ h2;
		}
	};

private:
	//===================================================================*/
	//                   private functions
	//===================================================================*/
	CollisionManager();
	~CollisionManager() = default;

	bool CheckCollisionPair(Collider* colliderA, Collider* colliderB);
	bool CanCollideByLayer(const Collider* colliderA, const Collider* colliderB) const;

	void  ComputeOBBAxes(const OBB& obb, CalyxEngine::Vector3 outAxis[3]);
	float ProjectOBB(const OBB& obb, const CalyxEngine::Vector3 obbAxes[3], const CalyxEngine::Vector3& axisCandidate);
	bool  OverlapOnAxis(
		 const OBB& obbA, const CalyxEngine::Vector3 aAxes[3],
		 const OBB& obbB, const CalyxEngine::Vector3 bAxes[3],
		 const CalyxEngine::Vector3& axisCandidate);

	/*----------------
	 各形状ごとの衝突
	----------------*/
	bool SphereToSphere(const Sphere& sphereA, const Sphere& sphereB);
	bool SphereToOBB(const Sphere& sphere, const OBB obb);
	bool OBBToOBB(const OBB& obbA, const OBB& obbB);
	bool SphereToCapsule(const Sphere& sphere, const Capsule& capsule);
	bool OBBToCapsule(const OBB& obb, const Capsule& capsule);
	bool CapsuleToCapsule(const Capsule& capsuleA, const Capsule& capsuleB);

	/*----------------
	 カプセル判定用の距離計算
	----------------*/
	void  GetCapsuleSegment(const Capsule& capsule, CalyxEngine::Vector3& outStart, CalyxEngine::Vector3& outEnd);
	float PointToSegmentDistanceSquared(const CalyxEngine::Vector3& point, const CalyxEngine::Vector3& segmentStart, const CalyxEngine::Vector3& segmentEnd);
	float SegmentToSegmentDistanceSquared(
		const CalyxEngine::Vector3& startA, const CalyxEngine::Vector3& endA,
		const CalyxEngine::Vector3& startB, const CalyxEngine::Vector3& endB);
	float SegmentToAABBDistanceSquared(
		const CalyxEngine::Vector3& segmentStart,
		const CalyxEngine::Vector3& segmentEnd,
		const CalyxEngine::Vector3& halfSize);

	void RegisterImmediate(Collider* collider);
	void UnregisterImmediate(Collider* collider);
	void FlushPendingColliderChanges();

private:
	//===================================================================*/
	//                   private variable
	//===================================================================*/
	std::list<Collider*> colliders_;                              //< 所有権を持たない判定対象Colliderの一覧
	std::vector<Collider*> pendingRegisters_;                     //< 判定中に要求された次回反映用の登録一覧
	std::vector<Collider*> pendingUnregisters_;                   //< 判定中に要求された次回反映用の登録解除一覧
	std::vector<std::string> collisionLogs_;                      //< Editorデバッグ表示用の衝突ログ
	std::unordered_set<CollisionPair, CollisionPairHash> currentCollisions_;  //< 現在フレームで接触中のペア
	std::unordered_set<CollisionPair, CollisionPairHash> previousCollisions_; //< 前フレームで接触していたペア
	bool isUpdatingCollisions_ = false;                           //< 判定一覧の走査中で遅延登録が必要か
};
