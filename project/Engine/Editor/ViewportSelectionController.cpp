#include "ViewportSelectionController.h"

#include <Engine/Application/UI/EngineUI/Viewport.h>
#include <Engine/Editor/PickingPass.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/System/SceneManager.h>

#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cmath>

namespace CalyxEngine {

	namespace {
		bool IsViewportSelectableObject(const SceneObject* object) {
			if(!object || object->IsTransient() || !object->IsPickable()) return false;
			if(object->GetObjectType() == ObjectType::Light) return false;
			if(object->GetObjectClassName() == "SkyBox") return false;
			if(object->GetName() == "ground") return false;
			return true;
		}
	} // namespace

	void ViewportSelectionController::CancelRangeSelection() {
		rangeSelectCandidate_ = false;
		rangeSelecting_ = false;
	}

	void ViewportSelectionController::UpdateInput() {
		if(!viewport_ || !viewport_->IsShow()) return;

		const CalyxEngine::Vector2 origin = viewport_->GetPosition();
		const CalyxEngine::Vector2 size = viewport_->GetSize();
		const ImVec2 mouse = ImGui::GetMousePos();
		const CalyxEngine::Vector2 local{mouse.x - origin.x, mouse.y - origin.y};
		const bool inViewport = local.x >= 0.0f && local.y >= 0.0f && local.x <= size.x && local.y <= size.y;

		if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) && inViewport) {
			rangeSelectCandidate_ = true;
			rangeSelecting_ = false;
			rangeSelectStart_ = local;
			rangeSelectEnd_ = local;
		}

		if(rangeSelectCandidate_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
			rangeSelectEnd_ = local;
			const CalyxEngine::Vector2 delta = rangeSelectEnd_ - rangeSelectStart_;
			if(delta.LengthSquared() > 36.0f) {
				rangeSelecting_ = true;
			}
		}

		if(rangeSelectCandidate_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if(rangeSelecting_) {
				SelectObjectsInViewportRect(rangeSelectStart_, rangeSelectEnd_, ImGui::GetIO().KeyCtrl);
			} else if(inViewport) {
				TryPickUnderCursor();
			}
			CancelRangeSelection();
		}
	}

	void ViewportSelectionController::DrawSelectionRect() const {
		if(!rangeSelecting_ || !viewport_) return;

		const CalyxEngine::Vector2 origin = viewport_->GetPosition();
		const ImVec2 a{origin.x + rangeSelectStart_.x, origin.y + rangeSelectStart_.y};
		const ImVec2 b{origin.x + rangeSelectEnd_.x, origin.y + rangeSelectEnd_.y};
		auto* drawList = ImGui::GetForegroundDrawList();
		drawList->AddRectFilled(a, b, IM_COL32(255, 160, 40, 45));
		drawList->AddRect(a, b, IM_COL32(255, 160, 40, 220), 0.0f, 0, 1.5f);
	}

	void ViewportSelectionController::TryPickObjectFromMouse(
		const CalyxEngine::Vector2& mouse,
		const CalyxEngine::Vector2& viewportSize,
		const CalyxEngine::Matrix4x4& view,
		const CalyxEngine::Matrix4x4& proj) {
		SceneContext* ctx = SceneContext::Current();
		if(!ctx || !viewport_) return;

		CalyxEngine::Vector2 mouseLocal = mouse - viewport_->GetPosition();
		Ray ray = Raycastor::ConvertMouseToRay(mouseLocal, view, proj, viewportSize);

		if(SceneObject* raw = PickSceneObjectByRay(ray)) {
			if(auto sp = ctx->FindSharedObject(raw)) {
				SelectObject(sp, ImGui::GetIO().KeyCtrl);
			}
		}
	}

	SceneObject* ViewportSelectionController::PickSceneObjectByRay(const Ray& ray) const {
		const auto* current = SceneContext::Current();
		const auto* lib = current ? current->GetObjectLibrary() : nullptr;
		if(!lib) return nullptr;

		const auto& allObjects = lib->GetAllObjectsRaw();
		std::vector<SceneObject*> pickableObjects;
		pickableObjects.reserve(allObjects.size());
		for(auto* object : allObjects) {
			if(IsViewportSelectableObject(object)) {
				pickableObjects.push_back(object);
			}
		}

		auto hit = Raycastor::Raycast(ray, pickableObjects);
		if(hit) {
			return static_cast<SceneObject*>(hit->hitObject);
		}
		return nullptr;
	}

	bool ViewportSelectionController::ProjectObjectToViewport(SceneObject* object, CalyxEngine::Vector2& outLocal) const {
		if(!object || !viewport_) return false;
		auto* camera = CameraManager::GetDebug();
		if(!camera) return false;

		const CalyxEngine::Vector3 worldPos = object->GetWorldTransform().GetWorldPosition();
		const CalyxEngine::Matrix4x4 viewProj = camera->GetViewProjectionMatrix();
		const CalyxEngine::Vector4 clip = CalyxEngine::Vector4::Transform(CalyxEngine::Vector4(worldPos, 1.0f), viewProj);
		if(std::abs(clip.w) <= 0.0001f) return false;

		const float ndcX = clip.x / clip.w;
		const float ndcY = clip.y / clip.w;
		const float ndcZ = clip.z / clip.w;
		if(ndcZ < 0.0f || ndcZ > 1.0f) return false;

		const CalyxEngine::Vector2 size = viewport_->GetSize();
		outLocal.x = (ndcX * 0.5f + 0.5f) * size.x;
		outLocal.y = (0.5f - ndcY * 0.5f) * size.y;
		return outLocal.x >= 0.0f && outLocal.y >= 0.0f && outLocal.x <= size.x && outLocal.y <= size.y;
	}

	void ViewportSelectionController::SelectObjectsInViewportRect(
		const CalyxEngine::Vector2& startLocal,
		const CalyxEngine::Vector2& endLocal,
		bool append) {
		SceneContext* current = SceneContext::Current();
		if(!current || !current->GetObjectLibrary()) return;

		const float minX = (std::min)(startLocal.x, endLocal.x);
		const float minY = (std::min)(startLocal.y, endLocal.y);
		const float maxX = (std::max)(startLocal.x, endLocal.x);
		const float maxY = (std::max)(startLocal.y, endLocal.y);

		std::vector<std::shared_ptr<SceneObject>> selected;
		if(append && callbacks_.getSelectedObjects) {
			selected = callbacks_.getSelectedObjects();
		}

		for(auto& object : current->GetObjectLibrary()->GetAllObjectsShared()) {
			if(!IsViewportSelectableObject(object.get())) continue;

			CalyxEngine::Vector2 projected;
			if(!ProjectObjectToViewport(object.get(), projected)) continue;
			if(projected.x < minX || projected.x > maxX || projected.y < minY || projected.y > maxY) continue;
			if(std::find(selected.begin(), selected.end(), object) != selected.end()) continue;
			selected.push_back(object);
		}

		if(callbacks_.setSelectedObjects) callbacks_.setSelectedObjects(selected);
	}

	void ViewportSelectionController::TryPickUnderCursor() {
		if(!viewport_ || !viewport_->IsShow()) return;

		SceneContext* current = SceneContext::Current();
		if(!current) return;

		CalyxEngine::Vector2 origin = viewport_->GetPosition();
		CalyxEngine::Vector2 size = viewport_->GetSize();

		ImVec2 mouse = ImGui::GetMousePos();
		float relativeX = mouse.x - origin.x;
		float relativeY = mouse.y - origin.y;

		if(relativeX < 0 || relativeY < 0 || relativeX > size.x || relativeY > size.y) return;

		if(sceneManager_) {
			if(auto* pickingPass = sceneManager_->GetPickingPass()) {
				float scaleX = static_cast<float>(pickingPass->GetWidth()) / size.x;
				float scaleY = static_cast<float>(pickingPass->GetHeight()) / size.y;

				int32_t px = static_cast<int32_t>(relativeX * scaleX);
				int32_t py = static_cast<int32_t>(relativeY * scaleY);

				uint32_t pickingID = pickingPass->GetObjectID(px, py);
				if(pickingID > 0) {
					if(auto sp = current->GetObjectLibrary()->FindSharedByPickingID(pickingID)) {
						if(IsViewportSelectableObject(sp.get())) {
							SelectObject(sp, ImGui::GetIO().KeyCtrl);
							return;
						}
					}
				}
			}
		}

		auto* debugCamera = CameraManager::GetDebug();
		if(!debugCamera) return;

		CalyxEngine::Vector2 mousePos(relativeX, relativeY);
		CalyxEngine::Matrix4x4 view = debugCamera->GetViewMatrix();
		CalyxEngine::Matrix4x4 proj = debugCamera->GetProjectionMatrix();

		Ray ray = Raycastor::ConvertMouseToRay(mousePos, view, proj, size);
		if(SceneObject* picked = PickSceneObjectByRay(ray)) {
			if(auto sp = current->FindSharedObject(picked)) {
				SelectObject(sp, ImGui::GetIO().KeyCtrl);
			}
		}
	}

	void ViewportSelectionController::SelectObject(const std::shared_ptr<SceneObject>& object, bool toggle) {
		if(!object) return;
		if(toggle) {
			if(callbacks_.toggleSelectedObject) callbacks_.toggleSelectedObject(object);
		} else {
			if(callbacks_.setSelectedObject) callbacks_.setSelectedObject(object);
		}
	}

} // namespace CalyxEngine
