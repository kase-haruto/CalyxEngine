#include "PhysicsSystem.h"

#include <Engine/Collision/CollisionManager.h>
#include <Engine/Objects/3D/Actor/Actor.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Collider/Collider.h>
#include <Engine/Physics/PhysicsBody.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace {
	struct ContactManifold {
		CalyxEngine::Vector3 normal{0.0f, 1.0f, 0.0f}; //< AをBから離す方向
		float penetration = 0.0f;						 //< 侵入量
	};

	bool IsNearlyZero(float value) {
		// 重み合計などを割り算に使う前に、浮動小数点の微小値を0扱いする。
		return std::abs(value) <= 1.0e-6f;
	}

	CalyxEngine::Vector3 SafeNormalize(const CalyxEngine::Vector3& v, const CalyxEngine::Vector3& fallback) {
		// 接触点が完全に重なっている場合は方向ベクトルが作れない。
		// そのまま正規化すると NaN が広がるため、呼び出し側が指定した代替方向を使う。
		if(v.LengthSquared() <= 1.0e-8f) {
			return fallback;
		}
		return v.Normalize();
	}

	void ComputeOBBAxes(const OBB& obb, CalyxEngine::Vector3 outAxes[3]) {
		outAxes[0] = CalyxEngine::Quaternion::RotateVector(CalyxEngine::Vector3::Right(), obb.rotate).Normalize();
		outAxes[1] = CalyxEngine::Quaternion::RotateVector(CalyxEngine::Vector3::Up(), obb.rotate).Normalize();
		outAxes[2] = CalyxEngine::Quaternion::RotateVector(CalyxEngine::Vector3::Forward(), obb.rotate).Normalize();
	}

	float ProjectOBB(const OBB& obb, const CalyxEngine::Vector3 axes[3], const CalyxEngine::Vector3& axis) {
		// OBBを任意軸へ射影したときの半径を求める。
		// 各ローカル軸の半サイズに、判定軸との内積絶対値を掛けて足し合わせる。
		const CalyxEngine::Vector3 halfSize = obb.size * 0.5f;
		return
			std::abs(halfSize.x * CalyxEngine::Vector3::Dot(axes[0], axis)) +
			std::abs(halfSize.y * CalyxEngine::Vector3::Dot(axes[1], axis)) +
			std::abs(halfSize.z * CalyxEngine::Vector3::Dot(axes[2], axis));
	}

	CalyxEngine::Vector3 ClosestPointOnSegment(
		const CalyxEngine::Vector3& point,
		const CalyxEngine::Vector3& segmentStart,
		const CalyxEngine::Vector3& segmentEnd) {
		// 線分の始点から終点への方向ベクトルを作る。
		const CalyxEngine::Vector3 segment = segmentEnd - segmentStart;
		const float segmentLengthSq = segment.LengthSquared();
		// 始点と終点が同じ場合、線分ではなく点として最近点を返す。
		if(segmentLengthSq <= 1.0e-8f) {
			return segmentStart;
		}
		// 点を線分方向へ射影し、線分範囲内に収まるよう t を 0..1 に丸める。
		const float t = std::clamp(CalyxEngine::Vector3::Dot(point - segmentStart, segment) / segmentLengthSq, 0.0f, 1.0f);
		// 丸めた t から線分上の最近点を復元する。
		return segmentStart + segment * t;
	}

	void GetCapsuleSegment(const Capsule& capsule, CalyxEngine::Vector3& outStart, CalyxEngine::Vector3& outEnd) {
		// カプセルは「中心線分 + 半径」として扱う。
		// height は半球込みの全体高さなので、中心線分の半長は height/2 - radius になる。
		const float radius = (std::max)(capsule.radius, 0.0f);
		const float halfSegment = (std::max)(0.0f, capsule.height * 0.5f - radius);
		// ローカルY軸をカプセル軸として、回転を反映したワールド軸を作る。
		const CalyxEngine::Vector3 axis = CalyxEngine::Quaternion::RotateVector(CalyxEngine::Vector3::Up(), capsule.rotate);
		// 中心から軸方向へ半長ぶん移動し、中心線分の両端を求める。
		outStart = capsule.center - axis * halfSegment;
		outEnd = capsule.center + axis * halfSegment;
	}

	CalyxEngine::Vector3 ClosestPointOnOBB(const OBB& obb, const CalyxEngine::Vector3& point) {
		CalyxEngine::Vector3 axes[3];
		ComputeOBBAxes(obb, axes);

		// OBB中心を基準に、点との差分を各ローカル軸へ射影する。
		CalyxEngine::Vector3 closest = obb.center;
		const CalyxEngine::Vector3 diff = point - obb.center;
		const CalyxEngine::Vector3 halfSize = obb.size * 0.5f;

		for(int i = 0; i < 3; ++i) {
			// 現在軸の半径を取得する。
			const float halfExtent = (i == 0) ? halfSize.x : (i == 1) ? halfSize.y : halfSize.z;
			// 射影距離を OBB の面内に clamp することで、最近点を箱の表面/内部に制限する。
			const float distance = std::clamp(CalyxEngine::Vector3::Dot(diff, axes[i]), -halfExtent, halfExtent);
			// 軸方向の最近点成分を加算する。
			closest += axes[i] * distance;
		}

		return closest;
	}

	bool SphereSphere(const Sphere& a, const Sphere& b, ContactManifold& out) {
		// A中心からB中心への差分を作る。法線は最終的に「AをBから離す方向」にする。
		const CalyxEngine::Vector3 diff = a.center - b.center;
		const float radiusSum = a.radius + b.radius;
		const float distSq = diff.LengthSquared();
		// 中心間距離が半径和より大きければ接触していない。
		if(distSq > radiusSum * radiusSum) return false;

		// 実距離を求め、半径和との差を侵入量として扱う。
		const float dist = std::sqrt((std::max)(distSq, 0.0f));
		out.normal = SafeNormalize(diff, CalyxEngine::Vector3::Up());
		out.penetration = radiusSum - dist;
		return out.penetration > 0.0f;
	}

	bool SphereOBB(const Sphere& sphere, const OBB& obb, ContactManifold& out) {
		// OBB上の最近点を取り、球中心との距離で接触を判定する。
		const CalyxEngine::Vector3 closest = ClosestPointOnOBB(obb, sphere.center);
		CalyxEngine::Vector3 diff = sphere.center - closest;
		float distSq = diff.LengthSquared();

		// 最近点が球半径の外側なら非接触。
		if(distSq > sphere.radius * sphere.radius) return false;

		if(distSq > 1.0e-8f) {
			// 球中心がOBBの外側にある通常ケース。
			// 最近点から球中心へ向かう方向が押し戻し方向になる。
			const float dist = std::sqrt(distSq);
			out.normal = diff / dist;
			out.penetration = sphere.radius - dist;
			return out.penetration > 0.0f;
		}

		// 球中心がOBB内部にいる場合は最近点との差分が0になり、方向が決められない。
		// そのため、OBBの各面までの距離を調べて最も近い面の外向き方向へ押し出す。
		CalyxEngine::Vector3 axes[3];
		ComputeOBBAxes(obb, axes);
		const CalyxEngine::Vector3 local = sphere.center - obb.center;
		const CalyxEngine::Vector3 halfSize = obb.size * 0.5f;
		float minExit = (std::numeric_limits<float>::max)();
		CalyxEngine::Vector3 bestNormal = axes[1];
		for(int i = 0; i < 3; ++i) {
			// 現在軸で見た球中心の位置を求める。
			const float halfExtent = (i == 0) ? halfSize.x : (i == 1) ? halfSize.y : halfSize.z;
			const float projected = CalyxEngine::Vector3::Dot(local, axes[i]);
			// 面までの残り距離を計算する。
			const float exitDistance = halfExtent - std::abs(projected);
			if(exitDistance < minExit) {
				// 最も近い面を採用し、中心が正側なら+軸、負側なら-軸へ押し出す。
				minExit = exitDistance;
				bestNormal = axes[i] * (projected >= 0.0f ? 1.0f : -1.0f);
			}
		}
		// 面まで出る距離に球半径を足し、球全体がOBB外へ出るようにする。
		out.normal = bestNormal;
		out.penetration = sphere.radius + minExit;
		return out.penetration > 0.0f;
	}

	bool OBBToOBB(const OBB& a, const OBB& b, ContactManifold& out) {
		// OBB同士は分離軸定理(SAT)で判定する。
		// 1本でも分離軸が見つかれば非接触、全軸で重なれば最小重なり軸を押し戻し方向にする。
		CalyxEngine::Vector3 aAxes[3];
		CalyxEngine::Vector3 bAxes[3];
		ComputeOBBAxes(a, aAxes);
		ComputeOBBAxes(b, bAxes);

		float minOverlap = (std::numeric_limits<float>::max)();
		CalyxEngine::Vector3 bestAxis = CalyxEngine::Vector3::Up();

		auto testAxis = [&](const CalyxEngine::Vector3& candidate) {
			// 平行な軸同士の外積などでゼロに近い軸ができるため、その軸は無視する。
			if(candidate.LengthSquared() <= 1.0e-8f) return true;

			CalyxEngine::Vector3 axis = candidate.Normalize();
			// 各OBBの中心を候補軸へ射影する。
			const float centerA = CalyxEngine::Vector3::Dot(a.center, axis);
			const float centerB = CalyxEngine::Vector3::Dot(b.center, axis);
			// それぞれの射影半径を足し、中心間距離を引くと重なり量になる。
			const float overlap = ProjectOBB(a, aAxes, axis) + ProjectOBB(b, bAxes, axis) - std::abs(centerA - centerB);
			// 重なりが0以下なら、この候補軸は分離軸なので接触していない。
			if(overlap <= 0.0f) return false;

			if(overlap < minOverlap) {
				// 最も浅い重なりを使うと、必要最小限の押し戻しになる。
				minOverlap = overlap;
				bestAxis = axis;
				// 法線を「AをBから離す方向」に揃える。
				if(CalyxEngine::Vector3::Dot(a.center - b.center, bestAxis) < 0.0f) {
					bestAxis = -bestAxis;
				}
			}
			return true;
		};

		for(int i = 0; i < 3; ++i) {
			// Aの3軸とBの3軸を候補軸として調べる。
			if(!testAxis(aAxes[i])) return false;
			if(!testAxis(bAxes[i])) return false;
		}
		for(int i = 0; i < 3; ++i) {
			for(int j = 0; j < 3; ++j) {
				// OBB同士では各軸の外積も候補軸になる。
				if(!testAxis(CalyxEngine::Vector3::Cross(aAxes[i], bAxes[j]))) return false;
			}
		}

		out.normal = bestAxis;
		out.penetration = minOverlap;
		return true;
	}

	bool SphereCapsule(const Sphere& sphere, const Capsule& capsule, ContactManifold& out) {
		// カプセルを中心線分として扱い、球中心から線分への最近点を求める。
		CalyxEngine::Vector3 start;
		CalyxEngine::Vector3 end;
		GetCapsuleSegment(capsule, start, end);
		const CalyxEngine::Vector3 closest = ClosestPointOnSegment(sphere.center, start, end);
		const CalyxEngine::Vector3 diff = sphere.center - closest;
		const float radiusSum = sphere.radius + capsule.radius;
		const float distSq = diff.LengthSquared();
		// 球半径 + カプセル半径の範囲外なら接触していない。
		if(distSq > radiusSum * radiusSum) return false;

		// 中心線分から球中心へ向かう方向を押し戻し方向にする。
		const float dist = std::sqrt((std::max)(distSq, 0.0f));
		out.normal = SafeNormalize(diff, CalyxEngine::Vector3::Up());
		out.penetration = radiusSum - dist;
		return out.penetration > 0.0f;
	}

	bool CapsuleCapsule(const Capsule& a, const Capsule& b, ContactManifold& out) {
		// カプセル同士は、中心線分同士の最近距離を半径和と比較する。
		CalyxEngine::Vector3 a0;
		CalyxEngine::Vector3 a1;
		CalyxEngine::Vector3 b0;
		CalyxEngine::Vector3 b1;
		GetCapsuleSegment(a, a0, a1);
		GetCapsuleSegment(b, b0, b1);

		// 反復回数を固定した近似で、2本の中心線の最近点を安定して求める。
		// A線分上を均等サンプルし、それぞれからB線分への最近点を調べる。
		float bestT = 0.0f;
		float bestU = 0.0f;
		float bestDistSq = (std::numeric_limits<float>::max)();
		for(int i = 0; i <= 8; ++i) {
			const float t = static_cast<float>(i) / 8.0f;
			const CalyxEngine::Vector3 p = a0 + (a1 - a0) * t;
			const CalyxEngine::Vector3 q = ClosestPointOnSegment(p, b0, b1);
			const float distSq = (p - q).LengthSquared();
			if(distSq < bestDistSq) {
				// 現時点で最も近いA線分上の位置を保存する。
				bestDistSq = distSq;
				bestT = t;
			}
		}
		// 保存したA線分上の位置から、B線分上の最近点を再取得する。
		const CalyxEngine::Vector3 p = a0 + (a1 - a0) * bestT;
		const CalyxEngine::Vector3 q = ClosestPointOnSegment(p, b0, b1);
		(void)bestU;

		const float radiusSum = a.radius + b.radius;
		// 中心線分間の距離が半径和より大きいなら接触していない。
		if(bestDistSq > radiusSum * radiusSum) return false;
		const float dist = std::sqrt((std::max)(bestDistSq, 0.0f));
		out.normal = SafeNormalize(p - q, CalyxEngine::Vector3::Up());
		out.penetration = radiusSum - dist;
		return out.penetration > 0.0f;
	}

	bool CapsuleOBB(const Capsule& capsule, const OBB& obb, ContactManifold& out) {
		CalyxEngine::Vector3 start;
		CalyxEngine::Vector3 end;
		GetCapsuleSegment(capsule, start, end);

		CalyxEngine::Vector3 axes[3];
		ComputeOBBAxes(obb, axes);
		const CalyxEngine::Vector3 halfSize = obb.size * 0.5f;

		float bestPenetration = (std::numeric_limits<float>::max)();
		CalyxEngine::Vector3 bestNormal = CalyxEngine::Vector3::Up();
		bool hasContact = false;

		for(int i = 0; i <= 8; ++i) {
			const float t = static_cast<float>(i) / 8.0f;
			const CalyxEngine::Vector3 p = start + (end - start) * t;
			const CalyxEngine::Vector3 q = ClosestPointOnOBB(obb, p);
			const float distSq = (p - q).LengthSquared();

			float penetration = 0.0f;
			CalyxEngine::Vector3 normal = CalyxEngine::Vector3::Up();

			if(distSq > 1.0e-8f) {
				if(distSq > capsule.radius * capsule.radius) continue;

				const float dist = std::sqrt((std::max)(distSq, 0.0f));
				normal = (p - q) / dist;
				penetration = capsule.radius - dist;
			} else {
				const CalyxEngine::Vector3 local = p - obb.center;
				float minExit = (std::numeric_limits<float>::max)();

				for(int axisIndex = 0; axisIndex < 3; ++axisIndex) {
					const float halfExtent = (axisIndex == 0) ? halfSize.x : (axisIndex == 1) ? halfSize.y : halfSize.z;
					const float projected = CalyxEngine::Vector3::Dot(local, axes[axisIndex]);
					const float exitDistance = halfExtent - std::abs(projected);

					if(exitDistance < minExit) {
						minExit = exitDistance;
						normal = axes[axisIndex] * (projected >= 0.0f ? 1.0f : -1.0f);
					}
				}

				penetration = capsule.radius + minExit;
			}

			if(penetration <= 0.0f || penetration >= bestPenetration) continue;

			bestPenetration = penetration;
			bestNormal = normal;
			hasContact = true;
		}

		if(!hasContact) return false;

		out.normal = SafeNormalize(bestNormal, CalyxEngine::Vector3::Up());
		out.penetration = bestPenetration;
		return out.penetration > 0.0f;
	}

	bool BuildContact(const CollisionShape& shapeA, const CollisionShape& shapeB, ContactManifold& out) {
		// variant に入っている実形状の組み合わせを見て、形状別の接触生成関数へ振り分ける。
		// out.normal は常に「AをBから離す方向」に揃える。
		return std::visit(
			[&](const auto& a, const auto& b) -> bool {
				using A = std::decay_t<decltype(a)>;
				using B = std::decay_t<decltype(b)>;

				if constexpr(std::is_same_v<A, Sphere> && std::is_same_v<B, Sphere>) {
					return SphereSphere(a, b, out);
				} else if constexpr(std::is_same_v<A, Sphere> && std::is_same_v<B, OBB>) {
					return SphereOBB(a, b, out);
				} else if constexpr(std::is_same_v<A, OBB> && std::is_same_v<B, Sphere>) {
					const bool hit = SphereOBB(b, a, out);
					// SphereOBB は Sphere 視点の法線を返すため、A=OBB 視点では向きを反転する。
					out.normal = -out.normal;
					return hit;
				} else if constexpr(std::is_same_v<A, OBB> && std::is_same_v<B, OBB>) {
					return OBBToOBB(a, b, out);
				} else if constexpr(std::is_same_v<A, Sphere> && std::is_same_v<B, Capsule>) {
					return SphereCapsule(a, b, out);
				} else if constexpr(std::is_same_v<A, Capsule> && std::is_same_v<B, Sphere>) {
					const bool hit = SphereCapsule(b, a, out);
					// SphereCapsule は Sphere 視点の法線を返すため、A=Capsule 視点では向きを反転する。
					out.normal = -out.normal;
					return hit;
				} else if constexpr(std::is_same_v<A, Capsule> && std::is_same_v<B, Capsule>) {
					return CapsuleCapsule(a, b, out);
				} else if constexpr(std::is_same_v<A, Capsule> && std::is_same_v<B, OBB>) {
					return CapsuleOBB(a, b, out);
				} else if constexpr(std::is_same_v<A, OBB> && std::is_same_v<B, Capsule>) {
					const bool hit = CapsuleOBB(b, a, out);
					// CapsuleOBB は Capsule 視点の法線を返すため、A=OBB 視点では向きを反転する。
					out.normal = -out.normal;
					return hit;
				} else {
					return false;
				}
			},
			shapeA,
			shapeB);
	}

	float MovableWeight(const PhysicsBody& body) {
		// Static は壁・床として扱い、押し戻しで動かさない。
		// Kinematic はキャラクターや移動オブジェクトとして、位置補正を受ける。
		return body.GetBodyType() == PhysicsBodyType::Kinematic ? 1.0f : 0.0f;
	}

	void ApplyCorrection(BaseGameObject* owner, Collider* collider, const CalyxEngine::Vector3& correction) {
		if(!owner || !collider) return;

		CalyxEngine::Vector3 adjustedCorrection = correction;
		if(dynamic_cast<Actor*>(owner) && adjustedCorrection.y > 0.0f) {
			adjustedCorrection.y = 0.0f;
		}
		if(adjustedCorrection.LengthSquared() <= 1.0e-10f) return;

		// Transform の座標を直接補正する。
		// この時点では各オブジェクトの更新後なので、次の描画に反映させるため行列も更新する。
		WorldTransform& transform = owner->GetWorldTransform();
		transform.translation += adjustedCorrection;
		transform.Update();

		// Transform を動かしたので、同じフレーム内の後続ペア判定が古い形状を使わないように
		// コライダー形状も即座に同期する。
		collider->Update(owner->GetCenterPos(), transform.rotation);
	}
}

PhysicsSystem* PhysicsSystem::GetInstance() {
	static PhysicsSystem instance;
	return &instance;
}

void PhysicsSystem::ResolveAll() {
	// CollisionManager の内部 list を直接走査すると、処理中の登録解除で iterator が崩れる。
	// ここではポインタ一覧をコピーして、現在フレームの固定スナップショットとして扱う。
	const auto colliders = CollisionManager::GetInstance()->GetCollidersSnapshot();

	for(size_t i = 0; i < colliders.size(); ++i) {
		Collider* a = colliders[i];
		// 無効コライダー、Trigger、nullptr は物理応答の対象外にする。
		if(!a || !a->IsCollisionEnubled() || a->IsTrigger()) continue;

		BaseGameObject* ownerA = a->GetOwner();
		// owner がないコライダーは Transform を補正できないため対象外。
		// PhysicsBody が無効なら、接触イベントだけを使うオブジェクトとして扱う。
		if(!ownerA || !ownerA->GetPhysicsBody().IsEnabled()) continue;

		for(size_t j = i + 1; j < colliders.size(); ++j) {
			Collider* b = colliders[j];
			// B側もA側と同じ基準で物理応答対象かを確認する。
			if(!b || !b->IsCollisionEnubled() || b->IsTrigger()) continue;

			BaseGameObject* ownerB = b->GetOwner();
			if(!ownerB || !ownerB->GetPhysicsBody().IsEnabled()) continue;

			PhysicsBody& bodyA = ownerA->GetPhysicsBody();
			PhysicsBody& bodyB = ownerB->GetPhysicsBody();
			// Body種別を「補正を受ける重み」に変換する。
			// Static/Static は合計0になり、壁同士のように動かす必要がない。
			const float weightA = MovableWeight(bodyA);
			const float weightB = MovableWeight(bodyB);
			const float totalWeight = weightA + weightB;
			if(IsNearlyZero(totalWeight)) continue;

			// 形状ペアから、押し戻し方向と侵入量を生成する。
			// 非接触なら補正せず次のペアへ進む。
			ContactManifold contact;
			if(!BuildContact(a->GetCollisionShape(), b->GetCollisionShape(), contact)) continue;
			if(contact.penetration <= 0.0f) continue;

			// 法線は A を B から離す方向なので、Aへは +normal、Bへは -normal を適用する。
			const CalyxEngine::Vector3 correction = contact.normal * contact.penetration;
			// 動ける重みの比率で補正量を分配する。
			// Kinematic vs Static は Kinematic が100%、Kinematic vs Kinematic は半分ずつになる。
			const float shareA = weightA / totalWeight;
			const float shareB = weightB / totalWeight;

			// PushbackRatio を最後に掛け、オブジェクトごとの補正の強さを調整できるようにする。
			ApplyCorrection(ownerA, a, correction * shareA * bodyA.GetPushbackRatio());
			ApplyCorrection(ownerB, b, -correction * shareB * bodyB.GetPushbackRatio());
		}
	}
}
