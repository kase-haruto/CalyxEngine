#include "CollisionManager.h"

// engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// lib
#include <algorithm>
#include <externals/imgui/imgui.h>


CollisionManager* CollisionManager::GetInstance() {
	static CollisionManager instance;
	return &instance;
}

// ヘルパー関数: 衝突ペアがログを記録すべきかを判定
bool CollisionManager::ShouldLogCollision(const Collider* a, const Collider* b) {
	// aのターゲットタイプにbのタイプが含まれているか
	bool aWantsToCollideWithB = (a->GetTargetType() & b->GetType()) != ColliderType::Type_None;

	// bのターゲットタイプにaのタイプが含まれているか
	bool bWantsToCollideWithA = (b->GetTargetType() & a->GetType()) != ColliderType::Type_None;

	return aWantsToCollideWithB || bWantsToCollideWithA;
}

void CollisionManager::UpdateCollisionAllCollider() {
	// 前フレームの衝突を保存
	previousCollisions_ = std::move(currentCollisions_);
	currentCollisions_.clear();

	for(auto itA = colliders_.begin(); itA != colliders_.end(); ++itA) {
		Collider* a = *itA;
		if(!a->IsCollisionEnubled()) continue;

		for(auto itB = std::next(itA); itB != colliders_.end(); ++itB) {
			Collider* b = *itB;
			if(!b->IsCollisionEnubled()) continue;

			//------------------------------------------------------------
			// 衝突対象タイプのフィルタリング（ビット安全比較）
			//------------------------------------------------------------
			bool aWantsB = (static_cast<uint32_t>(a->GetTargetType()) &
							static_cast<uint32_t>(b->GetType())) != 0u;
			bool bWantsA = (static_cast<uint32_t>(b->GetTargetType()) &
							static_cast<uint32_t>(a->GetType())) != 0u;

			// どちらも相手を対象にしていない場合 → 無視
			if(!(aWantsB || bWantsA)) continue;

			//------------------------------------------------------------
			// 実際の衝突判定
			//------------------------------------------------------------
			if(CheckCollisionPair(a, b)) {
				CollisionPair pair{a, b};
				currentCollisions_.insert(pair);

				if(previousCollisions_.find(pair) == previousCollisions_.end()) {
					a->NotifyCollisionEnter(b);
					b->NotifyCollisionEnter(a);
					collisionLogs_.emplace_back("Enter: " + a->GetName() + " VS " + b->GetName());
				} else {
					a->NotifyCollisionStay(b);
					b->NotifyCollisionStay(a);
				}
			}
		}
	}

	//------------------------------------------------------------
	// Exit判定
	//------------------------------------------------------------
	for(const auto& pair : previousCollisions_) {
		if(currentCollisions_.find(pair) == currentCollisions_.end()) {
			// 片方向でも対象ならExitを通知
			pair.a->NotifyCollisionExit(pair.b);
			pair.b->NotifyCollisionExit(pair.a);
			collisionLogs_.emplace_back("Exit: " + pair.a->GetName() + " VS " + pair.b->GetName());
		}
	}
}

void CollisionManager::Register(Collider* collider) {
	if(std::find(colliders_.begin(), colliders_.end(), collider) == colliders_.end()) {
		colliders_.push_back(collider);
	}
}

void CollisionManager::Unregister(Collider* collider) {
	std::erase(colliders_, collider);

	// 削除されるコライダーに関連する衝突ペアを現在のリストから削除
	std::erase_if(currentCollisions_, [collider](const CollisionPair& pair) {
		return pair.a == collider || pair.b == collider;
	});

	// 前フレームの衝突からも削除
	std::erase_if(previousCollisions_, [collider](const CollisionPair& pair) {
		return pair.a == collider || pair.b == collider;
	});
}

void CollisionManager::DebugLog() {

	// 衝突数を表示
	ImGui::Text("Colliders count: %zu", colliders_.size());
	ImGui::Text("Collisions detected: %zu", currentCollisions_.size());

	// スクロール可能なログフィールド
	ImGui::BeginChild("LogScroll", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
	for(const auto& log : collisionLogs_) {
		ImGui::TextUnformatted(log.c_str());
	}
	ImGui::EndChild();
}

void CollisionManager::ClearColliders() {
	colliders_.clear();
	collisionLogs_.clear();
	currentCollisions_.clear();
	previousCollisions_.clear();
}

bool CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {

	auto shapeA = colliderA->GetCollisionShape();
	auto shapeB = colliderB->GetCollisionShape();

	if(shapeA.index() > shapeB.index()) {
		std::swap(shapeA, shapeB);
	}

	return std::visit(
		[&](const auto& shapeA, const auto& shapeB) -> bool {
			using ShapeTypeA = std::decay_t<decltype(shapeA)>;
			using ShapeTypeB = std::decay_t<decltype(shapeB)>;

			//===================================================================*/
			//                    Sphere vs Sphere
			//===================================================================*/
			if constexpr(std::is_same_v<ShapeTypeA, Sphere> && std::is_same_v<ShapeTypeB, Sphere>) {
				return SphereToSphere(shapeA, shapeB);
			}

			//===================================================================*/
			//                    Sphere vs Obb
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA, Sphere> && std::is_same_v<ShapeTypeB, OBB>) {
				return SphereToOBB(shapeA, shapeB);
			}

			//===================================================================*/
			//                    OBB vs OBB
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA, OBB> && std::is_same_v<ShapeTypeB, OBB>) {
				return OBBToOBB(shapeA, shapeB);
			}

			// 設定していない組み合わせ
			else {
				return false;
			}
		},
		shapeA, shapeB);
}

bool CollisionManager::SphereToSphere(const Sphere& sphereA, const Sphere& sphereB) {
	const CalyxMath::Vector3& centerA	= sphereA.center;
	const CalyxMath::Vector3& centerB	= sphereB.center;
	float					  radiusSum = sphereA.radius + sphereB.radius;

	// 中心間距離の2乗を計算
	CalyxMath::Vector3 diff			   = centerA - centerB;
	float			   distanceSquared = diff.LengthSquared();

	// 衝突判定
	return distanceSquared <= (radiusSum * radiusSum);
}

bool CollisionManager::SphereToOBB(const Sphere& sphere, const OBB obb) {
	const CalyxMath::Vector3& sphereCenter = sphere.center;

	// CalyxMath::Quaternion から回転行列を作成
	CalyxMath::Matrix4x4 rotationMatrix = CalyxMath::Quaternion::ToMatrix(obb.rotate);

	// OBBの軸方向（ローカル軸 X, Y, Z）
	CalyxMath::Vector3 obbAxes[3] = {
		CalyxMath::Vector3::Transform(CalyxMath::Vector3(1.0f, 0.0f, 0.0f), rotationMatrix),
		CalyxMath::Vector3::Transform(CalyxMath::Vector3(0.0f, 1.0f, 0.0f), rotationMatrix),
		CalyxMath::Vector3::Transform(CalyxMath::Vector3(0.0f, 0.0f, 1.0f), rotationMatrix),
	};

	CalyxMath::Vector3 diff			= sphereCenter - obb.center;
	CalyxMath::Vector3 closestPoint = obb.center;

	for(int i = 0; i < 3; ++i) {
		float distance	 = CalyxMath::Vector3::Dot(diff, obbAxes[i]);
		float halfExtent = (i == 0) ? obb.size.x * 0.5f : (i == 1) ? obb.size.y * 0.5f
																   : obb.size.z * 0.5f;
		distance		 = std::clamp(distance, -halfExtent, halfExtent);
		closestPoint += distance * obbAxes[i];
	}

	CalyxMath::Vector3 closestToSphere = closestPoint - sphereCenter;
	float			   distanceSquared = closestToSphere.LengthSquared();
	return distanceSquared <= (sphere.radius * sphere.radius);
}

bool CollisionManager::OBBToOBB([[maybe_unused]] const OBB& obbA, [[maybe_unused]] const OBB& obbB) {
	// 1) A, B それぞれの 3軸ベクトル を求める
	CalyxMath::Vector3 aAxes[3];
	CalyxMath::Vector3 bAxes[3];
	ComputeOBBAxes(obbA, aAxes);
	ComputeOBBAxes(obbB, bAxes);

	// 2) 下記の全ての軸について OverlapOnAxis をチェック
	//    (1) Aの軸3本
	for(int i = 0; i < 3; i++) {
		if(!OverlapOnAxis(obbA, aAxes, obbB, bAxes, aAxes[i])) {
			return false; // 重ならない -> 分離軸
		}
	}
	//    (2) Bの軸3本
	for(int i = 0; i < 3; i++) {
		if(!OverlapOnAxis(obbA, aAxes, obbB, bAxes, bAxes[i])) {
			return false;
		}
	}
	//    (3) A.axis[i] × B.axis[j]  (i,j in [0..2])
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			CalyxMath::Vector3 crossAxis = CalyxMath::Vector3::Cross(aAxes[i], bAxes[j]);
			if(!OverlapOnAxis(obbA, aAxes, obbB, bAxes, crossAxis)) {
				return false;
			}
		}
	}

	// 3) すべての軸で重なった -> 衝突している
	return true;
}

void CollisionManager::ComputeOBBAxes(const OBB& obb, CalyxMath::Vector3 outAxis[3]) {
	CalyxMath::Matrix4x4 rot = CalyxMath::Quaternion::ToMatrix(obb.rotate); // CalyxMath::Quaternion → 回転行列

	outAxis[0] = CalyxMath::Vector3(rot.m[0][0], rot.m[0][1], rot.m[0][2]); // X軸
	outAxis[1] = CalyxMath::Vector3(rot.m[1][0], rot.m[1][1], rot.m[1][2]); // Y軸
	outAxis[2] = CalyxMath::Vector3(rot.m[2][0], rot.m[2][1], rot.m[2][2]); // Z軸

	outAxis[0].Normalize();
	outAxis[1].Normalize();
	outAxis[2].Normalize();
}

float CollisionManager::ProjectOBB(const OBB& obb, const CalyxMath::Vector3 obbAxes[3], const CalyxMath::Vector3& axisCandidate) {
	// OBBの半サイズ(各軸方向の半径)
	CalyxMath::Vector3 halfSize = obb.size * 0.5f;

	// 3つの軸に投影して絶対値を足し合わせる
	float r =
		fabs(halfSize.x * CalyxMath::Vector3::Dot(obbAxes[0], axisCandidate)) +
		fabs(halfSize.y * CalyxMath::Vector3::Dot(obbAxes[1], axisCandidate)) +
		fabs(halfSize.z * CalyxMath::Vector3::Dot(obbAxes[2], axisCandidate));

	return r;
}

bool CollisionManager::OverlapOnAxis(const OBB& obbA, const CalyxMath::Vector3 aAxes[3], const OBB& obbB, const CalyxMath::Vector3 bAxes[3], const CalyxMath::Vector3& axisCandidate) {
	// 軸が正規化されていないなら正規化しておく
	CalyxMath::Vector3 axis	 = axisCandidate;
	float			   lenSq = axis.LengthSquared();
	if(lenSq < 1e-8f) {
		// 軸がほぼゼロベクトルの場合は別の軸としてスキップ or 重なっているとみなす
		return true;
	}
	axis.Normalize();

	// A, B それぞれの投影中心(スカラー値)
	float centerA = CalyxMath::Vector3::Dot(obbA.center, axis);
	float centerB = CalyxMath::Vector3::Dot(obbB.center, axis);

	// A, B それぞれの投影半径
	float rA = ProjectOBB(obbA, aAxes, axis);
	float rB = ProjectOBB(obbB, bAxes, axis);

	// 中心距離
	float dist = fabs(centerB - centerA);

	// dist が rA + rB より大きい → 投影区間が重ならない
	return (dist <= (rA + rB));
}

CollisionManager::CollisionManager() {
	// 初期化処理
	collisionLogs_.clear();
}