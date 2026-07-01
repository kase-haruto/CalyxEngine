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
	constexpr int kPositionSolverIterations = 4;       //< 複数接触を安定させる位置Solverの反復回数
	constexpr float kPenetrationSlop = 0.001f;        //< 浮動小数点誤差として許容する微小な侵入量
	constexpr float kDynamicCorrectionPercent = 0.8f; //< Dynamicを含む接触で1反復に解消する侵入量の割合

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

	/**
	 * \brief 2本の線分上にある最近点の組を求める
	 * \param startA 線分Aの始点
	 * \param endA 線分Aの終点
	 * \param startB 線分Bの始点
	 * \param endB 線分Bの終点
	 * \param outPointA 線分A上の最近点
	 * \param outPointB 線分B上の最近点
	 */
	void ClosestPointsOnSegments(
		const CalyxEngine::Vector3& startA,
		const CalyxEngine::Vector3& endA,
		const CalyxEngine::Vector3& startB,
		const CalyxEngine::Vector3& endB,
		CalyxEngine::Vector3& outPointA,
		CalyxEngine::Vector3& outPointB) {
		// 各線分の方向と、始点同士を結ぶベクトルを求める。
		const CalyxEngine::Vector3 directionA = endA - startA;
		const CalyxEngine::Vector3 directionB = endB - startB;
		const CalyxEngine::Vector3 startDifference = startA - startB;
		const float lengthSqA = directionA.LengthSquared();
		const float lengthSqB = directionB.LengthSquared();
		const float directionBDotDifference = CalyxEngine::Vector3::Dot(directionB, startDifference);

		float parameterA = 0.0f;
		float parameterB = 0.0f;

		// 両方の線分が点まで縮退している場合は、各始点を最近点として返す。
		if(lengthSqA <= 1.0e-8f && lengthSqB <= 1.0e-8f) {
			outPointA = startA;
			outPointB = startB;
			return;
		}

		// Aだけが点の場合は、Aの点からB線分への射影位置を求める。
		if(lengthSqA <= 1.0e-8f) {
			parameterB = std::clamp(directionBDotDifference / lengthSqB, 0.0f, 1.0f);
		} else {
			const float directionADotDifference = CalyxEngine::Vector3::Dot(directionA, startDifference);

			// Bだけが点の場合は、Bの点からA線分への射影位置を求める。
			if(lengthSqB <= 1.0e-8f) {
				parameterA = std::clamp(-directionADotDifference / lengthSqA, 0.0f, 1.0f);
			} else {
				// 一般ケースでは、無限直線同士の最近点パラメータを求める。
				const float directionDot = CalyxEngine::Vector3::Dot(directionA, directionB);
				const float denominator = lengthSqA * lengthSqB - directionDot * directionDot;
				if(std::abs(denominator) > 1.0e-8f) {
					parameterA = std::clamp(
						(directionDot * directionBDotDifference - directionADotDifference * lengthSqB) / denominator,
						0.0f,
						1.0f);
				}

				// B側のパラメータを求め、端点を越えた場合はA側も計算し直す。
				parameterB = (directionDot * parameterA + directionBDotDifference) / lengthSqB;
				if(parameterB < 0.0f) {
					parameterB = 0.0f;
					parameterA = std::clamp(-directionADotDifference / lengthSqA, 0.0f, 1.0f);
				} else if(parameterB > 1.0f) {
					parameterB = 1.0f;
					parameterA = std::clamp((directionDot - directionADotDifference) / lengthSqA, 0.0f, 1.0f);
				}
			}
		}

		// 求めたパラメータから、各線分上の最近点を復元する。
		outPointA = startA + directionA * parameterA;
		outPointB = startB + directionB * parameterB;
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

		// 線分同士の最近点を解析的に求め、サンプル間隔による判定漏れを防ぐ。
		CalyxEngine::Vector3 p;
		CalyxEngine::Vector3 q;
		ClosestPointsOnSegments(a0, a1, b0, b1, p, q);
		const float bestDistSq = (p - q).LengthSquared();

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

		// 点からOBBまでの距離は、線分パラメータに対して凸になる。
		// 固定個数のサンプリングでは薄いBoxがサンプル間に入るため、線分全体から最近点を求める。
		auto distanceSquaredAt = [&](float t) {
			const CalyxEngine::Vector3 point = start + (end - start) * t;
			const CalyxEngine::Vector3 closest = ClosestPointOnOBB(obb, point);
			return (point - closest).LengthSquared();
		};

		float left = 0.0f;
		float right = 1.0f;
		for(int iteration = 0; iteration < 40; ++iteration) {
			const float t0 = left + (right - left) / 3.0f;
			const float t1 = right - (right - left) / 3.0f;
			if(distanceSquaredAt(t0) < distanceSquaredAt(t1)) {
				right = t1;
			} else {
				left = t0;
			}
		}

		const float closestT = (left + right) * 0.5f;
		const CalyxEngine::Vector3 p = start + (end - start) * closestT;
		const CalyxEngine::Vector3 q = ClosestPointOnOBB(obb, p);
		const float distSq = (p - q).LengthSquared();
		if(distSq > capsule.radius * capsule.radius) return false;

		if(distSq > 1.0e-8f) {
			const float dist = std::sqrt(distSq);
			out.normal = (p - q) / dist;
			out.penetration = capsule.radius - dist;
			return out.penetration > 0.0f;
		}

		// 中心線がOBB内部を通る場合は、最も近い面の外向き法線を使う。
		const CalyxEngine::Vector3 local = p - obb.center;
		float minExit = (std::numeric_limits<float>::max)();
		CalyxEngine::Vector3 normal = CalyxEngine::Vector3::Up();
		for(int axisIndex = 0; axisIndex < 3; ++axisIndex) {
			const float halfExtent = (axisIndex == 0) ? halfSize.x : (axisIndex == 1) ? halfSize.y : halfSize.z;
			const float projected = CalyxEngine::Vector3::Dot(local, axes[axisIndex]);
			const float exitDistance = halfExtent - std::abs(projected);
			if(exitDistance < minExit) {
				minExit = exitDistance;
				normal = axes[axisIndex] * (projected >= 0.0f ? 1.0f : -1.0f);
			}
		}

		out.normal = SafeNormalize(normal, CalyxEngine::Vector3::Up());
		out.penetration = capsule.radius + minExit;
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
		if(body.GetBodyType() == PhysicsBodyType::Kinematic) return 1.0f;
		// Dynamic同士では質量が軽いBodyほど大きく補正する。
		if(body.GetBodyType() == PhysicsBodyType::Dynamic) return body.GetInverseMass();
		return 0.0f;
	}

	/**
	 * \brief Dynamic Bodyの接近する法線速度をImpulseで打ち消す
	 * \param bodyA 接触Body A
	 * \param bodyB 接触Body B
	 * \param normal Body AをBody Bから離す接触法線
	 */
	void ResolveDynamicVelocity(
		PhysicsBody& bodyA,
		PhysicsBody& bodyB,
		const CalyxEngine::Vector3& normal) {
		// Dynamic以外は無限質量として扱い、速度補正の分母へ加えない。
		const float inverseMassA = bodyA.GetInverseMass();
		const float inverseMassB = bodyB.GetInverseMass();
		const float inverseMassSum = inverseMassA + inverseMassB;
		if(IsNearlyZero(inverseMassSum)) return;

		// Bから見たAの相対速度を接触法線へ射影する。
		const CalyxEngine::Vector3 relativeVelocity =
			bodyA.GetLinearVelocity() - bodyB.GetLinearVelocity();
		const float velocityAlongNormal = CalyxEngine::Vector3::Dot(relativeVelocity, normal);

		// 法線方向へ離れているBodyへImpulseを与えると引き戻してしまうため補正しない。
		if(velocityAlongNormal >= 0.0f) return;

		// 反発係数0の非弾性衝突として、接近する法線速度だけを打ち消す。
		const float impulseMagnitude = -velocityAlongNormal / inverseMassSum;
		const CalyxEngine::Vector3 impulse = normal * impulseMagnitude;

		// 逆質量に比例して、各Dynamic Bodyへ反対向きの速度変化を適用する。
		if(inverseMassA > 0.0f) bodyA.AddLinearVelocity(impulse * inverseMassA);
		if(inverseMassB > 0.0f) bodyB.AddLinearVelocity(-impulse * inverseMassB);
	}

	void ApplyCorrection(BaseGameObject* owner, Collider* collider, const CalyxEngine::Vector3& correction) {
		if(!owner || !collider) return;

		// オブジェクト種別によって補正軸を欠落させず、接触法線に沿った補正をそのまま適用する。
		// Character固有の床吸着はCharacterMovementComponent側で別途処理する。
		if(correction.LengthSquared() <= 1.0e-10f) return;

		// Transform の座標を直接補正する。
		// この時点では各オブジェクトの更新後なので、次の描画に反映させるため行列も更新する。
		WorldTransform& transform = owner->GetWorldTransform();
		transform.translation += correction;
		transform.Update();

		// Transform を動かしたので、同じフレーム内の後続ペア判定が古い形状を使わないように
		// コライダー形状も即座に同期する。
		collider->Update(owner->GetCenterPos(), transform.rotation);
	}

	void ResolveKinematicCharacterVelocity(BaseGameObject* owner, const CalyxEngine::Vector3& normal) {
		if(auto* actor = dynamic_cast<Actor*>(owner)) {
			actor->GetCharacterMovement().ResolveBlockingVelocity(normal);
			CalyxEngine::Vector3 velocity = actor->GetVelocity();
			const float velocityAlongNormal = CalyxEngine::Vector3::Dot(velocity, normal);
			if(velocityAlongNormal < 0.0f) {
				velocity -= normal * velocityAlongNormal;
				actor->SetVelocity(velocity);
			}
		}
	}
}

PhysicsSystem* PhysicsSystem::GetInstance() {
	static PhysicsSystem instance;
	return &instance;
}

void PhysicsSystem::Update(float deltaTime) {
	// 一時停止や不正な時間では物理時間を進めない。
	if(deltaTime <= 0.0f) return;

	// 大きなフレーム時間をそのまま蓄積すると復帰後に大量Stepが走るため上限を設ける。
	accumulator_ += (std::min)(deltaTime, 0.25f);

	int stepCount = 0;
	while(accumulator_ >= fixedDeltaTime_ && stepCount < maxSubSteps_) {
		// 固定幅だけ物理シミュレーションを進める。
		Step(fixedDeltaTime_);
		accumulator_ -= fixedDeltaTime_;
		++stepCount;
	}

	// 最大Step数を超えた古い時間は破棄し、処理落ちが連鎖するSpiral of Deathを防ぐ。
	if(stepCount == maxSubSteps_ && accumulator_ >= fixedDeltaTime_) {
		accumulator_ = 0.0f;
	}

}

void PhysicsSystem::Step(float fixedDeltaTime) {
	// 先に重力と速度からDynamicの予測位置を更新する。
	IntegrateDynamicBodies(fixedDeltaTime);

	// 押し戻し前の接触状態を保存し、この固定Stepの衝突イベントを通知する。
	CollisionManager::GetInstance()->UpdateCollisionAllCollider();

	// イベント確定後に、移動で発生しためり込みと法線方向速度を解決する。
	ResolveAll();
}

void PhysicsSystem::IntegrateDynamicBodies(float fixedDeltaTime) {
	const CalyxEngine::Vector3 gravity{0.0f, -9.8f, 0.0f};
	const auto colliders = CollisionManager::GetInstance()->GetCollidersSnapshot();

	for(Collider* collider : colliders) {
		// Colliderを持たないBodyは現在の登録方式では列挙されないため対象外とする。
		if(!collider || !collider->IsCollisionEnubled()) continue;

		BaseGameObject* owner = collider->GetOwner();
		if(!owner) continue;

		PhysicsBody& body = owner->GetPhysicsBody();
		if(!body.IsEnabled() || body.GetBodyType() != PhysicsBodyType::Dynamic) continue;

		// 重力を固定時間ぶん速度へ積分する。
		body.IntegrateForces(gravity, fixedDeltaTime);

		// 現在速度から固定時間ぶんの移動量を求めてTransformへ反映する。
		WorldTransform& transform = owner->GetWorldTransform();
		transform.translation += body.GetLinearVelocity() * fixedDeltaTime;
		transform.Update();

		// 後続の衝突判定が移動後の形状を使うようColliderを即座に同期する。
		collider->Update(owner->GetCenterPos(), transform.rotation);
	}
}

void PhysicsSystem::ResolveAll() {
	// CollisionManager の内部 list を直接走査すると、処理中の登録解除で iterator が崩れる。
	// ここではポインタ一覧をコピーして、現在フレームの固定スナップショットとして扱う。
	const auto colliders = CollisionManager::GetInstance()->GetCollidersSnapshot();

	// 1回の補正で別のColliderへ侵入する場合があるため、同じ一覧を複数回解決する。
	for(int iteration = 0; iteration < kPositionSolverIterations; ++iteration) {
		bool appliedAnyCorrection = false;

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
			float weightA = MovableWeight(bodyA);
			float weightB = MovableWeight(bodyB);

			// DynamicとKinematicの組み合わせでは、ゲーム制御のKinematicをSolverから動かさない。
			// Kinematic対StaticとKinematic対Kinematicは既存互換の位置補正を維持する。
			if(bodyA.GetBodyType() == PhysicsBodyType::Dynamic &&
			   bodyB.GetBodyType() == PhysicsBodyType::Kinematic) {
				weightB = 0.0f;
			} else if(bodyA.GetBodyType() == PhysicsBodyType::Kinematic &&
					  bodyB.GetBodyType() == PhysicsBodyType::Dynamic) {
				weightA = 0.0f;
			}
			const float totalWeight = weightA + weightB;
			if(IsNearlyZero(totalWeight)) continue;

			// 形状ペアから、押し戻し方向と侵入量を生成する。
			// 非接触なら補正せず次のペアへ進む。
			ContactManifold contact;
			if(!BuildContact(a->GetCollisionShape(), b->GetCollisionShape(), contact)) continue;

			// 速度補正は位置Solverの反復ごとに重複適用せず、最初の接触時だけ行う。
			if(iteration == 0) {
				ResolveDynamicVelocity(bodyA, bodyB, contact.normal);
				if(bodyA.GetBodyType() == PhysicsBodyType::Kinematic) {
					ResolveKinematicCharacterVelocity(ownerA, contact.normal);
				}
				if(bodyB.GetBodyType() == PhysicsBodyType::Kinematic) {
					ResolveKinematicCharacterVelocity(ownerB, -contact.normal);
				}
			}

			// 微小な侵入は許容し、接触面付近での補正振動を抑える。
			// 入力で直接移動するKinematicは次フレームにも壁へ押し込まれる。
			// Staticとの接触にDynamic用の緩和を掛けると残留侵入量が毎フレーム変動し、表示が振動するため、
			// この組み合わせだけは現在Step内で貫通量を全て解消する。
			const bool isKinematicStaticPair =
				(bodyA.GetBodyType() == PhysicsBodyType::Kinematic && bodyB.GetBodyType() == PhysicsBodyType::Static) ||
				(bodyA.GetBodyType() == PhysicsBodyType::Static && bodyB.GetBodyType() == PhysicsBodyType::Kinematic);
			const float correctionDepth = isKinematicStaticPair
				? contact.penetration + kPenetrationSlop
				: (std::max)(contact.penetration - kPenetrationSlop, 0.0f) * kDynamicCorrectionPercent;
			if(correctionDepth <= 0.0f) continue;

			// 法線は A を B から離す方向なので、Aへは +normal、Bへは -normal を適用する。
			const CalyxEngine::Vector3 correction = contact.normal * correctionDepth;
			// 動ける重みの比率で補正量を分配する。
			// Kinematic vs Static は Kinematic が100%、Kinematic vs Kinematic は半分ずつになる。
			const float shareA = weightA / totalWeight;
			const float shareB = weightB / totalWeight;

			// PushbackRatio を最後に掛け、オブジェクトごとの補正の強さを調整できるようにする。
			ApplyCorrection(ownerA, a, correction * shareA * bodyA.GetPushbackRatio());
			ApplyCorrection(ownerB, b, -correction * shareB * bodyB.GetPushbackRatio());
			appliedAnyCorrection = true;
			}
		}

		// すべての接触が許容侵入量以内なら、残りの反復を省略する。
		if(!appliedAnyCorrection) break;
	}
}
