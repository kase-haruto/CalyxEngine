#include "DebugCameraFocusController.h"

#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Transform/Transform.h>

#include <algorithm>
#include <cmath>

namespace CalyxEngine {
	namespace {
		float SmoothStep(float t) {
			t = std::clamp(t, 0.0f, 1.0f);
			return t * t * (3.0f - 2.0f * t);
		}

		Vector3 ExtractWorldForward(const WorldTransform& transform) {
			Vector3 forward{
				transform.matrix.world.m[2][0],
				transform.matrix.world.m[2][1],
				transform.matrix.world.m[2][2]};

			if(forward.LengthSquared() <= 0.000001f) {
				forward = transform.GetForward();
			}
			if(forward.LengthSquared() <= 0.000001f) {
				return Vector3::Forward();
			}
			return forward.Normalize();
		}

		DebugCamera::State LerpState(const DebugCamera::State& from, const DebugCamera::State& to, float t) {
			DebugCamera::State result = from;
			result.target			  = Vector3::Lerp(from.target, to.target, t);
			result.distance			  = Lerp(from.distance, to.distance, t);
			result.orbitAngle.x		  = LerpShortAngle(from.orbitAngle.x, to.orbitAngle.x, t);
			result.orbitAngle.y		  = LerpShortAngle(from.orbitAngle.y, to.orbitAngle.y, t);
			result.translation		  = Vector3::Lerp(from.translation, to.translation, t);
			result.eulerRotation.x	  = LerpShortAngle(from.eulerRotation.x, to.eulerRotation.x, t);
			result.eulerRotation.y	  = LerpShortAngle(from.eulerRotation.y, to.eulerRotation.y, t);
			result.eulerRotation.z	  = LerpShortAngle(from.eulerRotation.z, to.eulerRotation.z, t);
			return result;
		}

		Vector3 CalcOrbitOffset(float distance, const Vector2& orbitAngle) {
			Matrix4x4 matRotYaw	  = MakeRotateYMatrix(orbitAngle.x);
			Matrix4x4 matRotPitch = MakeRotateXMatrix(orbitAngle.y);
			Matrix4x4 matRot	  = Matrix4x4::Multiply(matRotPitch, matRotYaw);
			return TransformNormal(Vector3(0.0f, 0.0f, -distance), matRot);
		}
	} // namespace

	void DebugCameraFocusController::StartFocus(DebugCamera* camera, const std::shared_ptr<SceneObject>& object) {
		if(!camera || !object) {
			Cancel();
			return;
		}

		camera_	 = camera;
		start_	 = camera_->CaptureState();
		goal_	 = BuildFocusState(camera_, *object);
		elapsed_ = 0.0f;
		active_	 = true;
	}

	void DebugCameraFocusController::Update(float dt) {
		if(!active_ || !camera_) return;

		elapsed_ += (std::max)(dt, 0.0f);
		const float t = duration_ <= 0.0f ? 1.0f : SmoothStep(elapsed_ / duration_);
		camera_->ApplyState(LerpState(start_, goal_, t));

		if(t >= 1.0f) {
			active_ = false;
		}
	}

	void DebugCameraFocusController::Cancel() {
		camera_ = nullptr;
		active_ = false;
		elapsed_ = 0.0f;
	}

	DebugCamera::State DebugCameraFocusController::BuildFocusState(DebugCamera* camera, const SceneObject& object) const {
		DebugCamera::State state = camera ? camera->CaptureState() : DebugCamera::State{};

		const WorldTransform& transform = object.GetWorldTransform();
		const Vector3 target = transform.GetWorldPosition();
		const Vector3 objectForward = ExtractWorldForward(transform);
		const float distance = CalcFocusDistance(object);

		const Vector3 viewForward = (-objectForward).Normalize();
		const float pitchLimit = kPi * 0.5f - 0.01f;
		const float pitch = std::clamp(std::asin(-viewForward.y), -pitchLimit, pitchLimit);
		const float yaw = std::atan2(viewForward.x, viewForward.z);
		const Vector2 orbitAngle{yaw, pitch};

		state.target = target;
		state.distance = distance;
		state.orbitAngle = orbitAngle;
		state.translation = target + CalcOrbitOffset(distance, orbitAngle);
		state.eulerRotation = Vector3(pitch, yaw, 0.0f);
		return state;
	}

	float DebugCameraFocusController::CalcFocusDistance(const SceneObject& object) const {
		const Vector3 scale = object.GetWorldTransform().scale;
		const float scaleRadius = (std::max)({std::abs(scale.x), std::abs(scale.y), std::abs(scale.z)});
		return (std::max)(defaultDistance_, scaleRadius * 3.0f);
	}

} // namespace CalyxEngine
