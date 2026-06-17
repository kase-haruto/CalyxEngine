#include "CollisionManager.h"

// engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>

// lib
#include <algorithm>
#include <cmath>
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
	isUpdatingCollisions_ = true;

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

	isUpdatingCollisions_ = false;
	FlushPendingColliderChanges();
}

void CollisionManager::Register(Collider* collider) {
	if(isUpdatingCollisions_) {
		std::erase(pendingUnregisters_, collider);
		if(std::find(pendingRegisters_.begin(), pendingRegisters_.end(), collider) == pendingRegisters_.end()) {
			pendingRegisters_.push_back(collider);
		}
		return;
	}

	RegisterImmediate(collider);
}

void CollisionManager::Unregister(Collider* collider) {
	if(isUpdatingCollisions_) {
		std::erase(pendingRegisters_, collider);
		if(std::find(pendingUnregisters_.begin(), pendingUnregisters_.end(), collider) == pendingUnregisters_.end()) {
			pendingUnregisters_.push_back(collider);
		}
		return;
	}

	UnregisterImmediate(collider);
}

void CollisionManager::RegisterImmediate(Collider* collider) {
	if(std::find(colliders_.begin(), colliders_.end(), collider) == colliders_.end()) {
		colliders_.push_back(collider);
	}
}

void CollisionManager::UnregisterImmediate(Collider* collider) {
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

void CollisionManager::FlushPendingColliderChanges() {
	for(Collider* collider : pendingUnregisters_) {
		UnregisterImmediate(collider);
	}
	pendingUnregisters_.clear();

	for(Collider* collider : pendingRegisters_) {
		RegisterImmediate(collider);
	}
	pendingRegisters_.clear();
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
	pendingRegisters_.clear();
	pendingUnregisters_.clear();
	collisionLogs_.clear();
	currentCollisions_.clear();
	previousCollisions_.clear();
	isUpdatingCollisions_ = false;
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

			//===================================================================*/
			//                    球 vs カプセル
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA, Sphere> && std::is_same_v<ShapeTypeB, Capsule>) {
				return SphereToCapsule(shapeA, shapeB);
			}

			//===================================================================*/
			//                    OBB vs カプセル
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA, OBB> && std::is_same_v<ShapeTypeB, Capsule>) {
				return OBBToCapsule(shapeA, shapeB);
			}

			//===================================================================*/
			//                    カプセル vs カプセル
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA, Capsule> && std::is_same_v<ShapeTypeB, Capsule>) {
				return CapsuleToCapsule(shapeA, shapeB);
			}

			// 設定していない組み合わせ
			else {
				return false;
			}
		},
		shapeA, shapeB);
}

bool CollisionManager::SphereToSphere(const Sphere& sphereA, const Sphere& sphereB) {
	const CalyxEngine::Vector3& centerA	= sphereA.center;
	const CalyxEngine::Vector3& centerB	= sphereB.center;
	float					  radiusSum = sphereA.radius + sphereB.radius;

	// 中心間距離の2乗を計算
	CalyxEngine::Vector3 diff			   = centerA - centerB;
	float			   distanceSquared = diff.LengthSquared();

	// 衝突判定
	return distanceSquared <= (radiusSum * radiusSum);
}

bool CollisionManager::SphereToOBB(const Sphere& sphere, const OBB obb) {
	const CalyxEngine::Vector3& sphereCenter = sphere.center;

	// CalyxEngine::Quaternion から回転行列を作成
	CalyxEngine::Matrix4x4 rotationMatrix = CalyxEngine::Quaternion::ToMatrix(obb.rotate);

	// OBBの軸方向（ローカル軸 X, Y, Z）
	CalyxEngine::Vector3 obbAxes[3] = {
		CalyxEngine::Vector3::Transform(CalyxEngine::Vector3(1.0f, 0.0f, 0.0f), rotationMatrix),
		CalyxEngine::Vector3::Transform(CalyxEngine::Vector3(0.0f, 1.0f, 0.0f), rotationMatrix),
		CalyxEngine::Vector3::Transform(CalyxEngine::Vector3(0.0f, 0.0f, 1.0f), rotationMatrix),
	};

	CalyxEngine::Vector3 diff			= sphereCenter - obb.center;
	CalyxEngine::Vector3 closestPoint = obb.center;

	for(int i = 0; i < 3; ++i) {
		float distance	 = CalyxEngine::Vector3::Dot(diff, obbAxes[i]);
		float halfExtent = (i == 0) ? obb.size.x * 0.5f : (i == 1) ? obb.size.y * 0.5f
																   : obb.size.z * 0.5f;
		distance		 = std::clamp(distance, -halfExtent, halfExtent);
		closestPoint += distance * obbAxes[i];
	}

	CalyxEngine::Vector3 closestToSphere = closestPoint - sphereCenter;
	float			   distanceSquared = closestToSphere.LengthSquared();
	return distanceSquared <= (sphere.radius * sphere.radius);
}

bool CollisionManager::OBBToOBB([[maybe_unused]] const OBB& obbA, [[maybe_unused]] const OBB& obbB) {
	// 1) A, B それぞれの 3軸ベクトル を求める
	CalyxEngine::Vector3 aAxes[3];
	CalyxEngine::Vector3 bAxes[3];
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
			CalyxEngine::Vector3 crossAxis = CalyxEngine::Vector3::Cross(aAxes[i], bAxes[j]);
			if(!OverlapOnAxis(obbA, aAxes, obbB, bAxes, crossAxis)) {
				return false;
			}
		}
	}

	// 3) すべての軸で重なった -> 衝突している
	return true;
}

bool CollisionManager::SphereToCapsule(const Sphere& sphere, const Capsule& capsule) {
	CalyxEngine::Vector3 segmentStart;
	CalyxEngine::Vector3 segmentEnd;
	GetCapsuleSegment(capsule, segmentStart, segmentEnd);

	const float radiusSum = sphere.radius + capsule.radius;
	return PointToSegmentDistanceSquared(sphere.center, segmentStart, segmentEnd) <= radiusSum * radiusSum;
}

bool CollisionManager::OBBToCapsule(const OBB& obb, const Capsule& capsule) {
	CalyxEngine::Vector3 segmentStart;
	CalyxEngine::Vector3 segmentEnd;
	GetCapsuleSegment(capsule, segmentStart, segmentEnd);

	// OBBローカルへ線分を移し、AABBと線分の距離で判定する
	const CalyxEngine::Quaternion invRot = CalyxEngine::Quaternion::Inverse(obb.rotate);
	const CalyxEngine::Vector3 localStart = CalyxEngine::Quaternion::RotateVector(segmentStart - obb.center, invRot);
	const CalyxEngine::Vector3 localEnd = CalyxEngine::Quaternion::RotateVector(segmentEnd - obb.center, invRot);
	const CalyxEngine::Vector3 halfSize = obb.size * 0.5f;

	return SegmentToAABBDistanceSquared(localStart, localEnd, halfSize) <= capsule.radius * capsule.radius;
}

bool CollisionManager::CapsuleToCapsule(const Capsule& capsuleA, const Capsule& capsuleB) {
	CalyxEngine::Vector3 startA;
	CalyxEngine::Vector3 endA;
	CalyxEngine::Vector3 startB;
	CalyxEngine::Vector3 endB;
	GetCapsuleSegment(capsuleA, startA, endA);
	GetCapsuleSegment(capsuleB, startB, endB);

	const float radiusSum = capsuleA.radius + capsuleB.radius;
	return SegmentToSegmentDistanceSquared(startA, endA, startB, endB) <= radiusSum * radiusSum;
}

void CollisionManager::GetCapsuleSegment(const Capsule& capsule, CalyxEngine::Vector3& outStart, CalyxEngine::Vector3& outEnd) {
	const float radius = (std::max)(capsule.radius, 0.0f);
	const float halfSegment = (std::max)(0.0f, capsule.height * 0.5f - radius);
	const CalyxEngine::Vector3 axis = CalyxEngine::Quaternion::RotateVector(CalyxEngine::Vector3::Up(), capsule.rotate);

	outStart = capsule.center - axis * halfSegment;
	outEnd = capsule.center + axis * halfSegment;
}

float CollisionManager::PointToSegmentDistanceSquared(const CalyxEngine::Vector3& point, const CalyxEngine::Vector3& segmentStart, const CalyxEngine::Vector3& segmentEnd) {
	const CalyxEngine::Vector3 segment = segmentEnd - segmentStart;
	const float segmentLengthSq = segment.LengthSquared();
	if(segmentLengthSq <= 1.0e-8f) {
		return (point - segmentStart).LengthSquared();
	}

	const float t = std::clamp(CalyxEngine::Vector3::Dot(point - segmentStart, segment) / segmentLengthSq, 0.0f, 1.0f);
	const CalyxEngine::Vector3 closest = segmentStart + segment * t;
	return (point - closest).LengthSquared();
}

float CollisionManager::SegmentToSegmentDistanceSquared(
	const CalyxEngine::Vector3& startA, const CalyxEngine::Vector3& endA,
	const CalyxEngine::Vector3& startB, const CalyxEngine::Vector3& endB) {
	const CalyxEngine::Vector3 d1 = endA - startA;
	const CalyxEngine::Vector3 d2 = endB - startB;
	const CalyxEngine::Vector3 r = startA - startB;
	const float a = d1.LengthSquared();
	const float e = d2.LengthSquared();
	const float f = CalyxEngine::Vector3::Dot(d2, r);

	float s = 0.0f;
	float t = 0.0f;

	if(a <= 1.0e-8f && e <= 1.0e-8f) {
		return (startA - startB).LengthSquared();
	}
	if(a <= 1.0e-8f) {
		t = std::clamp(f / e, 0.0f, 1.0f);
	} else {
		const float c = CalyxEngine::Vector3::Dot(d1, r);
		if(e <= 1.0e-8f) {
			s = std::clamp(-c / a, 0.0f, 1.0f);
		} else {
			const float b = CalyxEngine::Vector3::Dot(d1, d2);
			const float denom = a * e - b * b;
			if(denom != 0.0f) {
				s = std::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
			}

			t = (b * s + f) / e;
			if(t < 0.0f) {
				t = 0.0f;
				s = std::clamp(-c / a, 0.0f, 1.0f);
			} else if(t > 1.0f) {
				t = 1.0f;
				s = std::clamp((b - c) / a, 0.0f, 1.0f);
			}
		}
	}

	const CalyxEngine::Vector3 closestA = startA + d1 * s;
	const CalyxEngine::Vector3 closestB = startB + d2 * t;
	return (closestA - closestB).LengthSquared();
}

float CollisionManager::SegmentToAABBDistanceSquared(
	const CalyxEngine::Vector3& segmentStart,
	const CalyxEngine::Vector3& segmentEnd,
	const CalyxEngine::Vector3& halfSize) {
	auto pointToAABBDistanceSquared = [&](const CalyxEngine::Vector3& point) {
		const float dx = (std::max)(std::abs(point.x) - halfSize.x, 0.0f);
		const float dy = (std::max)(std::abs(point.y) - halfSize.y, 0.0f);
		const float dz = (std::max)(std::abs(point.z) - halfSize.z, 0.0f);
		return dx * dx + dy * dy + dz * dz;
	};

	// 線分とAABBの距離は凸関数になるため、三分探索で最近点を安定して求める
	float left = 0.0f;
	float right = 1.0f;
	const CalyxEngine::Vector3 segment = segmentEnd - segmentStart;
	for(int i = 0; i < 32; ++i) {
		const float t0 = left + (right - left) / 3.0f;
		const float t1 = right - (right - left) / 3.0f;
		const float d0 = pointToAABBDistanceSquared(segmentStart + segment * t0);
		const float d1 = pointToAABBDistanceSquared(segmentStart + segment * t1);
		if(d0 < d1) {
			right = t1;
		} else {
			left = t0;
		}
	}

	const float t = (left + right) * 0.5f;
	return pointToAABBDistanceSquared(segmentStart + segment * t);
}

void CollisionManager::ComputeOBBAxes(const OBB& obb, CalyxEngine::Vector3 outAxis[3]) {
	CalyxEngine::Matrix4x4 rot = CalyxEngine::Quaternion::ToMatrix(obb.rotate); // CalyxEngine::Quaternion → 回転行列

	outAxis[0] = CalyxEngine::Vector3(rot.m[0][0], rot.m[0][1], rot.m[0][2]); // X軸
	outAxis[1] = CalyxEngine::Vector3(rot.m[1][0], rot.m[1][1], rot.m[1][2]); // Y軸
	outAxis[2] = CalyxEngine::Vector3(rot.m[2][0], rot.m[2][1], rot.m[2][2]); // Z軸

	outAxis[0].Normalize();
	outAxis[1].Normalize();
	outAxis[2].Normalize();
}

float CollisionManager::ProjectOBB(const OBB& obb, const CalyxEngine::Vector3 obbAxes[3], const CalyxEngine::Vector3& axisCandidate) {
	// OBBの半サイズ(各軸方向の半径)
	CalyxEngine::Vector3 halfSize = obb.size * 0.5f;

	// 3つの軸に投影して絶対値を足し合わせる
	float r =
		fabs(halfSize.x * CalyxEngine::Vector3::Dot(obbAxes[0], axisCandidate)) +
		fabs(halfSize.y * CalyxEngine::Vector3::Dot(obbAxes[1], axisCandidate)) +
		fabs(halfSize.z * CalyxEngine::Vector3::Dot(obbAxes[2], axisCandidate));

	return r;
}

bool CollisionManager::OverlapOnAxis(const OBB& obbA, const CalyxEngine::Vector3 aAxes[3], const OBB& obbB, const CalyxEngine::Vector3 bAxes[3], const CalyxEngine::Vector3& axisCandidate) {
	// 軸が正規化されていないなら正規化しておく
	CalyxEngine::Vector3 axis	 = axisCandidate;
	float			   lenSq = axis.LengthSquared();
	if(lenSq < 1e-8f) {
		// 軸がほぼゼロベクトルの場合は別の軸としてスキップ or 重なっているとみなす
		return true;
	}
	axis.Normalize();

	// A, B それぞれの投影中心(スカラー値)
	float centerA = CalyxEngine::Vector3::Dot(obbA.center, axis);
	float centerB = CalyxEngine::Vector3::Dot(obbB.center, axis);

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
