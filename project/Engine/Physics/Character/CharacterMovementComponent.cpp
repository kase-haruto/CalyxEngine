#include "CharacterMovementComponent.h"

#include <Engine/Collision/CollisionManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Collider/BoxCollider.h>
#include <Engine/Objects/Collider/CapsuleCollider.h>
#include <Engine/Objects/Collider/Collider.h>
#include <Engine/Objects/Collider/SphereCollider.h>
#include <Engine/Physics/PhysicsBody.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
	constexpr float kPi = 3.14159265358979323846f;

	float ToRadians(float degree) {
		return degree * kPi / 180.0f;
	}

	CalyxEngine::Vector3 SafeNormalize(const CalyxEngine::Vector3& v, const CalyxEngine::Vector3& fallback) {
		// 長さがほぼ0のベクトルを正規化すると NaN の原因になる。
		// 床法線など、方向が必要な場面では明示的な代替方向を使う。
		if(v.LengthSquared() <= 1.0e-8f) {
			return fallback;
		}
		return v.Normalize();
	}

	void ComputeOBBAxes(const OBB& obb, CalyxEngine::Vector3 outAxes[3]) {
		// OBBの回転を行列へ変換し、ローカルXYZ軸をワールド空間へ取り出す。
		const CalyxEngine::Matrix4x4 rot = CalyxEngine::Quaternion::ToMatrix(obb.rotate);
		outAxes[0] = CalyxEngine::Vector3(rot.m[0][0], rot.m[0][1], rot.m[0][2]).Normalize();
		outAxes[1] = CalyxEngine::Vector3(rot.m[1][0], rot.m[1][1], rot.m[1][2]).Normalize();
		outAxes[2] = CalyxEngine::Vector3(rot.m[2][0], rot.m[2][1], rot.m[2][2]).Normalize();
	}

	bool RaycastOBBDown(
		const CalyxEngine::Vector3& rayStart,
		float rayLength,
		const OBB& obb,
		float characterRadius,
		float& outDistance,
		CalyxEngine::Vector3& outPoint,
		CalyxEngine::Vector3& outNormal) {
		CalyxEngine::Vector3 axes[3];
		ComputeOBBAxes(obb, axes);

		// キャラクターの足元幅を考慮するため、床探索時だけOBBをXZ方向へ少し膨らませる。
		// 厳密なCapsuleCastではないが、Planeに厚み付きBoxColliderを置いた初期段階では安定する。
		CalyxEngine::Vector3 halfSize = obb.size * 0.5f;
		halfSize.x += characterRadius;
		halfSize.z += characterRadius;

		// レイ始点をOBBローカルへ移す。
		const CalyxEngine::Vector3 relative = rayStart - obb.center;
		const CalyxEngine::Vector3 localOrigin = {
			CalyxEngine::Vector3::Dot(relative, axes[0]),
			CalyxEngine::Vector3::Dot(relative, axes[1]),
			CalyxEngine::Vector3::Dot(relative, axes[2])};

		// ワールド下方向もOBBローカルへ変換する。
		const CalyxEngine::Vector3 worldDown = {0.0f, -1.0f, 0.0f};
		const CalyxEngine::Vector3 localDir = {
			CalyxEngine::Vector3::Dot(worldDown, axes[0]),
			CalyxEngine::Vector3::Dot(worldDown, axes[1]),
			CalyxEngine::Vector3::Dot(worldDown, axes[2])};

		float tMin = 0.0f;
		float tMax = rayLength;
		int hitAxis = -1;
		float hitAxisSign = 1.0f;

		for(int axisIndex = 0; axisIndex < 3; ++axisIndex) {
			const float origin = localOrigin[axisIndex];
			const float dir = localDir[axisIndex];
			const float extent = halfSize[axisIndex];

			if(std::abs(dir) <= 1.0e-6f) {
				// レイがこの軸と平行な場合、始点がスラブ外なら交差しない。
				if(origin < -extent || origin > extent) {
					return false;
				}
				continue;
			}

			// AABBスラブ法で、この軸の進入距離と退出距離を求める。
			float t1 = (-extent - origin) / dir;
			float t2 = (extent - origin) / dir;
			float sign = -1.0f;
			if(t1 > t2) {
				std::swap(t1, t2);
				sign = 1.0f;
			}

			// 最も遅い進入距離が最終的なヒット距離になる。
			if(t1 > tMin) {
				tMin = t1;
				hitAxis = axisIndex;
				hitAxisSign = sign;
			}
			tMax = (std::min)(tMax, t2);

			// 進入距離が退出距離を超えたら交差しない。
			if(tMin > tMax) {
				return false;
			}
		}

		if(tMin < 0.0f || tMin > rayLength) {
			return false;
		}

		outDistance = tMin;
		outPoint = rayStart + worldDown * tMin;

		// スラブ進入軸から法線を復元する。
		// レイ始点が箱内部にあるケースでは軸が取れないため、上方向を代替にする。
		if(hitAxis >= 0) {
			outNormal = axes[hitAxis] * hitAxisSign;
		} else {
			outNormal = CalyxEngine::Vector3::Up();
		}
		outNormal = SafeNormalize(outNormal, CalyxEngine::Vector3::Up());
		return true;
	}
}

void CharacterMovementComponent::Tick(float dt) {
	if(!owner_) return;

	// 入力方向は前フレームから蓄積されている可能性があるため、最初に正規化して使う。
	CalyxEngine::Vector3 moveInput = pendingInput_;
	pendingInput_ = CalyxEngine::Vector3::Zero();
	moveInput.y = 0.0f;
	if(moveInput.LengthSquared() > 1.0f) {
		moveInput = moveInput.Normalize();
	}

	FindFloor(currentFloor_);

	if(currentFloor_.walkableFloor && currentFloor_.floorDistance <= floorSnapDistance_ && velocity_.y <= 0.0f) {
		// 歩行可能な床が近くにある場合はWalkingへ遷移する。
		movementMode_ = CharacterMovementMode::Walking;
		// 接地中は下向き速度を残すと毎フレーム床へ押し込み続けるため0に戻す。
		velocity_.y = 0.0f;
		// 入力を速度へ変換し、水平移動だけを先に反映する。
		// 縦方向は床吸着が担当するため、ここではYを動かさない。
		WorldTransform& transform = owner_->GetWorldTransform();
		transform.translation += moveInput * maxWalkSpeed_ * dt;
		transform.Update();
		// 床との微小な隙間や段差を安定させるため、床へ吸着する。
		SnapToFloor();
		return;
	}

	// 床が見つからない、または歩行不能な斜面ならFallingへ遷移する。
	movementMode_ = CharacterMovementMode::Falling;
	// 重力で縦方向速度を増やす。速度は下方向なのでyを減らす。
	velocity_.y -= gravity_ * dt;
	// 極端な落下速度で薄い床を抜けやすくならないよう最大速度を制限する。
	velocity_.y = (std::max)(velocity_.y, -maxFallSpeed_);

	WorldTransform& transform = owner_->GetWorldTransform();
	// 空中でも水平入力は受け付ける。
	// まず水平移動を反映し、その後に重力でY方向を動かす。
	transform.translation += moveInput * maxWalkSpeed_ * dt;
	transform.translation.y += velocity_.y * dt;
	transform.Update();
}

void CharacterMovementComponent::AddMovementInput(const CalyxEngine::Vector3& worldDirection, float scale) {
	// 入力がない場合やスケールが0の場合は何もしない。
	if(worldDirection.LengthSquared() <= 1.0e-8f || std::abs(scale) <= 1.0e-6f) return;

	// 入力方向を水平面に制限する。
	// キャラクター歩行では上下方向の入力を受け付けず、ジャンプがY方向を担当する。
	CalyxEngine::Vector3 direction = worldDirection;
	direction.y = 0.0f;
	if(direction.LengthSquared() <= 1.0e-8f) return;

	// 複数方向の入力を合成できるよう pendingInput_ に蓄積する。
	pendingInput_ += direction.Normalize() * scale;
}

void CharacterMovementComponent::Jump() {
	// 接地中だけジャンプできる。
	// 空中で連続して呼ばれても二段ジャンプにならないようにする。
	if(movementMode_ != CharacterMovementMode::Walking) return;

	// 上方向速度を設定して Falling に遷移する。
	// 次の Tick から重力がこの速度を減速させる。
	velocity_.y = jumpVelocity_;
	movementMode_ = CharacterMovementMode::Falling;
	currentFloor_ = {};
}

void CharacterMovementComponent::FindFloor(FindFloorResult& outFloor) const {
	outFloor = {};
	if(!owner_) return;

	float capsuleRadius = 0.0f;
	float capsuleHalfHeight = 0.0f;
	if(!GetOwnerCapsule(capsuleRadius, capsuleHalfHeight)) return;

	// UnrealのFindFloorに寄せ、キャラクターの衝突中心から下方向へ床を探す。
	// BaseGameObjectはコライダー更新時にGetCenterPos()を使うため、
	// 床探索も同じ基準点を使わないと、表示されているカプセル底面とFloorDistanceがずれる。
	const CalyxEngine::Vector3 collisionCenter = owner_->GetCenterPos();
	const CalyxEngine::Vector3 rayStart = collisionCenter;
	const float rayLength = capsuleHalfHeight + floorProbeDistance_;

	float bestDistance = (std::numeric_limits<float>::max)();
	Collider* bestCollider = nullptr;
	CalyxEngine::Vector3 bestPoint{};
	CalyxEngine::Vector3 bestNormal{};

	const auto colliders = CollisionManager::GetInstance()->GetCollidersSnapshot();
	for(Collider* collider : colliders) {
		if(!collider) continue;
		if(collider == owner_->GetCollider()) continue;
		if(!collider->IsCollisionEnubled() || collider->IsTrigger()) continue;

		BaseGameObject* otherOwner = collider->GetOwner();
		if(!otherOwner) continue;
		if(otherOwner->GetPhysicsBody().GetBodyType() != PhysicsBodyType::Static) continue;

		float hitDistance = 0.0f;
		CalyxEngine::Vector3 hitPoint{};
		CalyxEngine::Vector3 hitNormal{};
		bool hit = false;

		const CollisionShape& shape = collider->GetCollisionShape();
		if(const OBB* obb = std::get_if<OBB>(&shape)) {
			hit = RaycastOBBDown(rayStart, rayLength, *obb, capsuleRadius, hitDistance, hitPoint, hitNormal);
		}

		if(!hit) continue;
		if(hitDistance >= bestDistance) continue;

		bestDistance = hitDistance;
		bestCollider = collider;
		bestPoint = hitPoint;
		bestNormal = hitNormal;
	}

	if(!bestCollider) return;

	outFloor.blockingHit = true;
	outFloor.hitCollider = bestCollider;
	outFloor.hitPoint = bestPoint;
	outFloor.hitNormal = bestNormal;
	outFloor.floorDistance = bestDistance - capsuleHalfHeight;
	outFloor.walkableFloor = IsWalkable(bestNormal);
}

void CharacterMovementComponent::ShowGui() {
	GuiCmd::DragFloat("Gravity", gravity_, 0.1f, 0.0f, 100.0f);
	GuiCmd::DragFloat("Max Fall Speed", maxFallSpeed_, 0.1f, 0.0f, 1000.0f);
	GuiCmd::DragFloat("Walkable Floor Angle", walkableFloorAngle_, 0.1f, 0.0f, 89.0f);
	GuiCmd::DragFloat("Floor Probe Distance", floorProbeDistance_, 0.01f, 0.0f, 10.0f);
	GuiCmd::DragFloat("Floor Snap Distance", floorSnapDistance_, 0.01f, 0.0f, 10.0f);
	GuiCmd::DragFloat("Skin Width", skinWidth_, 0.001f, 0.0f, 1.0f);
	GuiCmd::DragFloat("Max Walk Speed", maxWalkSpeed_, 0.1f, 0.0f, 100.0f);
	GuiCmd::DragFloat("Jump Velocity", jumpVelocity_, 0.1f, 0.0f, 100.0f);
}

bool CharacterMovementComponent::GetOwnerCapsule(float& outRadius, float& outHalfHeight) const {
	if(!owner_) return false;

	Collider* collider = owner_->GetCollider();
	if(!collider) return false;

	if(auto* capsule = dynamic_cast<CapsuleCollider*>(collider)) {
		outRadius = capsule->GetRadius();
		outHalfHeight = capsule->GetHeight() * 0.5f;
		return true;
	}

	if(auto* sphere = dynamic_cast<SphereCollider*>(collider)) {
		outRadius = sphere->GetColliderRadius();
		outHalfHeight = outRadius;
		return true;
	}

	if(auto* box = dynamic_cast<BoxCollider*>(collider)) {
		const CalyxEngine::Vector3 size = box->GetSize();
		outRadius = (std::max)(size.x, size.z) * 0.5f;
		outHalfHeight = size.y * 0.5f;
		return true;
	}

	return false;
}

bool CharacterMovementComponent::IsWalkable(const CalyxEngine::Vector3& normal) const {
	const CalyxEngine::Vector3 up = CalyxEngine::Vector3::Up();
	const float minFloorDot = std::cos(ToRadians(walkableFloorAngle_));
	return CalyxEngine::Vector3::Dot(SafeNormalize(normal, up), up) >= minFloorDot;
}

void CharacterMovementComponent::SnapToFloor() {
	if(!owner_) return;
	if(!currentFloor_.walkableFloor) return;

	// floorDistance はカプセル底面から床までの距離。
	// 0より大きければ浮いているので下げ、0より小さければめり込んでいるので上げる。
	const float desiredDistance = skinWidth_;
	const float correctionY = -(currentFloor_.floorDistance - desiredDistance);

	if(std::abs(correctionY) <= 1.0e-5f) return;

	WorldTransform& transform = owner_->GetWorldTransform();
	transform.translation.y += correctionY;
	transform.Update();

	if(Collider* collider = owner_->GetCollider()) {
		collider->Update(owner_->GetCenterPos(), transform.rotation);
	}
}
