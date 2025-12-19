#include "CollisionManager.h"

// engine
#include <Engine/Foundation/Utility/Func/MyFunc.h>

//lib
#include <externals/imgui/imgui.h>
#include <algorithm>

CollisionManager* CollisionManager::GetInstance() {
	static CollisionManager instance;
	return &instance;
}

std::string CollisionManager::MakeCollisionKey(Collider* colliderA,Collider* colliderB) {

	// 名前を使ってユニークなキーを生成（順序を保証するために名前をソート）
	return (colliderA->GetName() < colliderB->GetName())
			   ? colliderA->GetName() + " VS " + colliderB->GetName()
			   : colliderB->GetName() + " VS " + colliderA->GetName();

}

// ヘルパー関数: 衝突ペアがログを記録すべきかを判定
bool CollisionManager::ShouldLogCollision(const Collider* a,const Collider* b) {
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
			if(CheckCollisionPair(a,b)) {
				std::string key = MakeCollisionKey(a,b);
				currentCollisions_.insert(key);

				if(previousCollisions_.find(key) == previousCollisions_.end()) {
					a->NotifyCollisionEnter(b);
					b->NotifyCollisionEnter(a);
					collisionLogs_.emplace_back("Enter: " + key);
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
	for(const auto& key : previousCollisions_) {
		if(currentCollisions_.find(key) == currentCollisions_.end()) {
			auto delimiterPos = key.find(" VS ");
			if(delimiterPos == std::string::npos) continue;

			std::string colliderAName = key.substr(0,delimiterPos);
			std::string colliderBName = key.substr(delimiterPos + 4);

			Collider* colliderA = FindColliderByName(colliderAName);
			Collider* colliderB = FindColliderByName(colliderBName);

			if(!colliderA || !colliderB) continue;

			// ターゲットフィルタ再チェック（安全化済み）
			bool aWantsB = (static_cast<uint32_t>(colliderA->GetTargetType()) &
							static_cast<uint32_t>(colliderB->GetType())) != 0u;
			bool bWantsA = (static_cast<uint32_t>(colliderB->GetTargetType()) &
							static_cast<uint32_t>(colliderA->GetType())) != 0u;

			if(!(aWantsB || bWantsA)) continue;

			// Exit 通知
			colliderA->NotifyCollisionExit(colliderB);
			colliderB->NotifyCollisionExit(colliderA);
			collisionLogs_.emplace_back("Exit: " + key);
		}
	}
}

Collider* CollisionManager::FindColliderByName(const std::string& name) {
	for(auto* collider : colliders_) { if(collider->GetName() == name) { return collider; } }
	return nullptr; // 見つからない場合
}

void CollisionManager::Register(Collider* collider) { if(std::find(colliders_.begin(),colliders_.end(),collider) == colliders_.end()) { colliders_.push_back(collider); } }

void CollisionManager::Unregister(Collider* collider) { std::erase(colliders_,collider); }

void CollisionManager::DebugLog() {

	// 衝突数を表示
	ImGui::Text("Colliders count: %zu",colliders_.size());
	ImGui::Text("Collisions detected: %zu",currentCollisions_.size());

	// スクロール可能なログフィールド
	ImGui::BeginChild("LogScroll",ImVec2(0,200),true,ImGuiWindowFlags_HorizontalScrollbar);
	for(const auto& log : collisionLogs_) { ImGui::TextUnformatted(log.c_str()); }
	ImGui::EndChild();

}

void CollisionManager::ClearColliders() {
	colliders_.clear();
	collisionLogs_.clear();
	currentCollisions_.clear();
	previousCollisions_.clear();
}

bool CollisionManager::CheckCollisionPair(Collider* colliderA,Collider* colliderB) {

	auto shapeA = colliderA->GetCollisionShape();
	auto shapeB = colliderB->GetCollisionShape();

	if(shapeA.index() > shapeB.index()) { std::swap(shapeA,shapeB); }

	return std::visit(
		[&](const auto& shapeA,const auto& shapeB) -> bool {
			using ShapeTypeA = std::decay_t<decltype(shapeA)>;
			using ShapeTypeB = std::decay_t<decltype(shapeB)>;

			//===================================================================*/
			//                    Sphere vs Sphere
			//===================================================================*/
			if constexpr(std::is_same_v<ShapeTypeA,Sphere> && std::is_same_v<ShapeTypeB,Sphere>) { return SphereToSphere(shapeA,shapeB); }

			//===================================================================*/
			//                    Sphere vs Obb
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA,Sphere> && std::is_same_v<ShapeTypeB,OBB>) { return SphereToOBB(shapeA,shapeB); }

			//===================================================================*/
			//                    OBB vs OBB
			//===================================================================*/
			else if constexpr(std::is_same_v<ShapeTypeA,OBB> && std::is_same_v<ShapeTypeB,OBB>) { return OBBToOBB(shapeA,shapeB); }

			// 設定していない組み合わせ
			else { return false; }
		},shapeA,shapeB);
}

bool CollisionManager::SphereToSphere(const Sphere& sphereA,const Sphere& sphereB) {
	const CxMath::Vector3& centerA   = sphereA.center;
	const CxMath::Vector3& centerB   = sphereB.center;
	float          radiusSum = sphereA.radius + sphereB.radius;

	// 中心間距離の2乗を計算
	CxMath::Vector3 diff            = centerA - centerB;
	float   distanceSquared = diff.LengthSquared();

	// 衝突判定
	return distanceSquared <= (radiusSum * radiusSum);

}

bool CollisionManager::SphereToOBB(const Sphere& sphere,const OBB obb) {
	const CxMath::Vector3& sphereCenter = sphere.center;

	// CxMath::Quaternion から回転行列を作成
	CxMath::Matrix4x4 rotationMatrix = CxMath::Quaternion::ToMatrix(obb.rotate);

	// OBBの軸方向（ローカル軸 X, Y, Z）
	CxMath::Vector3 obbAxes[3] = {
			CxMath::Vector3::Transform(CxMath::Vector3(1.0f,0.0f,0.0f),rotationMatrix),
			CxMath::Vector3::Transform(CxMath::Vector3(0.0f,1.0f,0.0f),rotationMatrix),
			CxMath::Vector3::Transform(CxMath::Vector3(0.0f,0.0f,1.0f),rotationMatrix),
		};

	CxMath::Vector3 diff         = sphereCenter - obb.center;
	CxMath::Vector3 closestPoint = obb.center;

	for(int i = 0; i < 3; ++i) {
		float distance   = CxMath::Vector3::Dot(diff,obbAxes[i]);
		float halfExtent = (i == 0) ? obb.size.x * 0.5f : (i == 1) ? obb.size.y * 0.5f : obb.size.z * 0.5f;
		distance         = std::clamp(distance,-halfExtent,halfExtent);
		closestPoint += distance * obbAxes[i];
	}

	CxMath::Vector3 closestToSphere = closestPoint - sphereCenter;
	float   distanceSquared = closestToSphere.LengthSquared();
	return distanceSquared <= (sphere.radius * sphere.radius);
}


bool CollisionManager::OBBToOBB([[maybe_unused]] const OBB& obbA,[[maybe_unused]] const OBB& obbB) {
	// 1) A, B それぞれの 3軸ベクトル を求める
	CxMath::Vector3 aAxes[3];
	CxMath::Vector3 bAxes[3];
	ComputeOBBAxes(obbA,aAxes);
	ComputeOBBAxes(obbB,bAxes);

	// 2) 下記の全ての軸について OverlapOnAxis をチェック
	//    (1) Aの軸3本
	for(int i = 0; i < 3; i++) {
		if(!OverlapOnAxis(obbA,aAxes,obbB,bAxes,aAxes[i])) {
			return false; // 重ならない -> 分離軸
		}
	}
	//    (2) Bの軸3本
	for(int i = 0; i < 3; i++) { if(!OverlapOnAxis(obbA,aAxes,obbB,bAxes,bAxes[i])) { return false; } }
	//    (3) A.axis[i] × B.axis[j]  (i,j in [0..2])
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			CxMath::Vector3 crossAxis = CxMath::Vector3::Cross(aAxes[i],bAxes[j]);
			if(!OverlapOnAxis(obbA,aAxes,obbB,bAxes,crossAxis)) { return false; }
		}
	}

	// 3) すべての軸で重なった -> 衝突している
	return true;
}

void CollisionManager::ComputeOBBAxes(const OBB& obb,CxMath::Vector3 outAxis[3]) {
	CxMath::Matrix4x4 rot = CxMath::Quaternion::ToMatrix(obb.rotate); // CxMath::Quaternion → 回転行列

	outAxis[0] = CxMath::Vector3(rot.m[0][0],rot.m[0][1],rot.m[0][2]); // X軸
	outAxis[1] = CxMath::Vector3(rot.m[1][0],rot.m[1][1],rot.m[1][2]); // Y軸
	outAxis[2] = CxMath::Vector3(rot.m[2][0],rot.m[2][1],rot.m[2][2]); // Z軸

	outAxis[0].Normalize();
	outAxis[1].Normalize();
	outAxis[2].Normalize();
}

float CollisionManager::ProjectOBB(const OBB& obb,const CxMath::Vector3 obbAxes[3],const CxMath::Vector3& axisCandidate) {
	// OBBの半サイズ(各軸方向の半径)
	CxMath::Vector3 halfSize = obb.size * 0.5f;

	// 3つの軸に投影して絶対値を足し合わせる
	float r =
		fabs(halfSize.x * CxMath::Vector3::Dot(obbAxes[0],axisCandidate)) +
		fabs(halfSize.y * CxMath::Vector3::Dot(obbAxes[1],axisCandidate)) +
		fabs(halfSize.z * CxMath::Vector3::Dot(obbAxes[2],axisCandidate));

	return r;
}

bool CollisionManager::OverlapOnAxis(const OBB& obbA,const CxMath::Vector3 aAxes[3],const OBB& obbB,const CxMath::Vector3 bAxes[3],const CxMath::Vector3& axisCandidate) {
	// 軸が正規化されていないなら正規化しておく
	CxMath::Vector3 axis  = axisCandidate;
	float   lenSq = axis.LengthSquared();
	if(lenSq < 1e-8f) {
		// 軸がほぼゼロベクトルの場合は別の軸としてスキップ or 重なっているとみなす
		return true;
	}
	axis.Normalize();

	// A, B それぞれの投影中心(スカラー値)
	float centerA = CxMath::Vector3::Dot(obbA.center,axis);
	float centerB = CxMath::Vector3::Dot(obbB.center,axis);

	// A, B それぞれの投影半径
	float rA = ProjectOBB(obbA,aAxes,axis);
	float rB = ProjectOBB(obbB,bAxes,axis);

	// 中心距離
	float dist = fabs(centerB - centerA);

	// dist が rA + rB より大きい → 投影区間が重ならない
	return (dist <= (rA + rB));
}


CollisionManager::CollisionManager() {
	// 初期化処理
	collisionLogs_.clear();
}