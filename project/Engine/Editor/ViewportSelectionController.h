#pragma once

#include <memory>
#include <vector>
#include <functional>

#include <Engine/Foundation/Math/Vector2.h>

class SceneObject;

namespace CalyxEngine {
	class SceneManager;
	struct Matrix4x4;
	class Viewport;
}

struct Ray;

namespace CalyxEngine {

	struct ViewportSelectionCallbacks {
		std::function<std::vector<std::shared_ptr<SceneObject>>()> getSelectedObjects;
		std::function<void(const std::shared_ptr<SceneObject>&)> setSelectedObject;
		std::function<void(const std::shared_ptr<SceneObject>&)> toggleSelectedObject;
		std::function<void(const std::vector<std::shared_ptr<SceneObject>>&)> setSelectedObjects;
	};

	class ViewportSelectionController {
	public:
		void SetViewport(Viewport* viewport) { viewport_ = viewport; }
		void SetSceneManager(SceneManager* sceneManager) { sceneManager_ = sceneManager; }
		void SetCallbacks(ViewportSelectionCallbacks callbacks) { callbacks_ = std::move(callbacks); }

		void CancelRangeSelection();
		void UpdateInput();
		void DrawSelectionRect() const;

	private:
		void TryPickUnderCursor();
		void TryPickObjectFromMouse(const CalyxEngine::Vector2& mouse,
									const CalyxEngine::Vector2& viewportSize,
									const CalyxEngine::Matrix4x4& view,
									const CalyxEngine::Matrix4x4& proj);
		SceneObject* PickSceneObjectByRay(const Ray& ray) const;
		bool ProjectObjectToViewport(SceneObject* object, CalyxEngine::Vector2& outLocal) const;
		void SelectObjectsInViewportRect(const CalyxEngine::Vector2& startLocal,
										 const CalyxEngine::Vector2& endLocal,
										 bool append);
		void SelectObject(const std::shared_ptr<SceneObject>& object, bool toggle);

		Viewport* viewport_ = nullptr;
		SceneManager* sceneManager_ = nullptr;
		ViewportSelectionCallbacks callbacks_;
		bool rangeSelectCandidate_ = false;
		bool rangeSelecting_ = false;
		CalyxEngine::Vector2 rangeSelectStart_{};
		CalyxEngine::Vector2 rangeSelectEnd_{};
	};

} // namespace CalyxEngine
