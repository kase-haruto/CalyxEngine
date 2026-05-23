#pragma once

#include <Engine/Graphics/Camera/3d/DebugCamera.h>

#include <memory>

class SceneObject;

namespace CalyxEngine {

	class DebugCameraFocusController {
	public:
		void StartFocus(DebugCamera* camera, const std::shared_ptr<SceneObject>& object);
		void Update(float dt);
		void Cancel();

		bool IsActive() const { return active_; }

	private:
		DebugCamera::State BuildFocusState(DebugCamera* camera, const SceneObject& object) const;
		float CalcFocusDistance(const SceneObject& object) const;

	private:
		DebugCamera* camera_ = nullptr;
		DebugCamera::State start_{};
		DebugCamera::State goal_{};
		float elapsed_ = 0.0f;
		float duration_ = 0.35f;
		float defaultDistance_ = 10.0f;
		bool active_ = false;
	};

} // namespace CalyxEngine
