#include "Viewport.h"
/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/ImGuizmo.h>

namespace CalyxEditor {

	Viewport::Viewport(ViewportType type, const std::string& windowName)
		: IEngineUI(windowName), type_(type), windowName_(windowName) {}

	void Viewport::Update() {}

	void Viewport::Render(const ImTextureID& tex) {

		switch(type_) {
		case ViewportType::VIEWPORT_MAIN: {
			auto* mainCam = CameraManager::GetMain3d();
			if(mainCam && camera_ != mainCam) {
				camera_ = mainCam;
			}
			break;
		}
		case ViewportType::VIEWPORT_DEBUG:
		case ViewportType::VIEWPORT_PICKING: {
			auto* debugCam = CameraManager::GetDebug();
			if(debugCam && camera_ != debugCam) {
				camera_ = debugCam;
			}
			break;
		}
		}

		if(!camera_) {
			return;
		}

		textureID_ = tex;

		bool open = true;

		ImGui::Begin(windowName_.c_str(), &open,
					 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImVec2 contentSize = ImGui::GetContentRegionAvail();
		size_			   = CalyxMath::Vector2(contentSize.x, contentSize.y);
		// 画像描画
		ImVec2 imagePos = ImGui::GetCursorScreenPos();
		viewOrigin_		= CalyxMath::Vector2(imagePos.x, imagePos.y);
		// 描画サイズをカメラに通知 (PICKINGビューポートは表示のみなのでカメラ設定を上書きしない)
		if(size_.y > 0.0f && type_ != ViewportType::VIEWPORT_PICKING) {
			camera_->SetAspectRatio(size_.x / size_.y);
			camera_->UpdateMatrix();
			// CameraManagerにviewportサイズを通知
			CameraManager::SetViewportSizeStatic(type_, size_);
		}

		ImGui::SetCursorScreenPos(imagePos);
		ImGui::Image(textureID_, contentSize);

		// --- Drag and Drop Target ---
		if(ImGui::BeginDragDropTarget()) {
			if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PLACE_ITEM")) {
				const PlaceToolPanel::PlaceItem* item = *(const PlaceToolPanel::PlaceItem**)payload->Data;
				if(item) {
					// マウス位置からレイを生成
					ImVec2			   mousePos = ImGui::GetMousePos();
					CalyxMath::Vector2 localMousePos(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

					Ray ray = Raycastor::ConvertMouseToRay(
						localMousePos,
						camera_->GetViewMatrix(),
						camera_->GetProjectionMatrix(),
						size_);

					// シーン内のオブジェクトと衝突判定
					SceneContext*	   ctx		= SceneContext::Current();
					CalyxMath::Vector3 spawnPos = CalyxMath::Vector3::Zero();

					if(ctx) {
						auto allObjects = ctx->GetObjectLibrary()->GetAllObjectsRaw();
						auto hit		= Raycastor::Raycast(ray, allObjects);

						if(hit) {
							spawnPos = hit->point;
						} else {
							// ヒットしなければ一定距離先に配置
							spawnPos = ray.origin + ray.direction * 10.0f;
						}
					}

					// オブジェクト作成
					item->createFunc(spawnPos);
				}
			}
			ImGui::EndDragDropTarget();
		}

		if(type_ == ViewportType::VIEWPORT_DEBUG) {
			ImGuizmo::SetRect(imagePos.x, imagePos.y, size_.x, size_.y);
			ImGuizmo::SetDrawlist();

			ImVec2 viewportPos	= imagePos;
			ImVec2 viewportSize = ImVec2(size_.x, size_.y);

			ImGui::BeginGroup();
			for(auto* tool : tools_) {
				auto* base = dynamic_cast<BaseOnViewportTool*>(tool);
				if(!base) continue;

				ImVec2 viewSize = ImVec2(size_.x, size_.y);

				ImVec2 pos = base->CalcScreenPosition(imagePos, viewSize);
				tool->RenderOverlay(pos);
			}

			ImGui::EndGroup();
		}

		isHovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
		isClicked_ = ImGui::IsWindowFocused() && ImGui::IsMouseDown(0);
		isClicked_ = ImGui::IsWindowFocused() && CalyxFoundation::Input::GetInstance()->TriggerMouseButton(CalyxFoundation::MouseButton::Left);

		ImGui::End();
		if(!open) {
			SetShow(false);
		}
	}

	ImVec2 Viewport::CalcToolPosition(IOnViewportTool* tool, const ImVec2& viewportPos, const ImVec2& viewportSize) {
		ImVec2 basePos;

		OverlayAlign align = OverlayAlign::TopLeft;
		if(auto* base = dynamic_cast<BaseOnViewportTool*>(tool)) {
			align = base->GetOverlayAlign();
		}

		switch(align) {
		case OverlayAlign::TopLeft:
			basePos = viewportPos;
			break;
		case OverlayAlign::TopRight:
			basePos = ImVec2(viewportPos.x + viewportSize.x, viewportPos.y);
			break;
		case OverlayAlign::BottomLeft:
			basePos = ImVec2(viewportPos.x, viewportPos.y + viewportSize.y);
			break;
		case OverlayAlign::BottomRight:
			basePos = ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y);
			break;
		case OverlayAlign::CenterTop:
			basePos = ImVec2(viewportPos.x + viewportSize.x * 0.5f, viewportPos.y);
			break;
		}

		// オフセット加算
		return ImVec2(basePos.x + tool->GetOverlayOffset().x,
					  basePos.y + tool->GetOverlayOffset().y);
	}

	void Viewport::AddTool(IOnViewportTool* tool) { tools_.push_back(tool); }

	bool			   Viewport::IsHovered() const { return isHovered_; }
	bool			   Viewport::IsClicked() const { return isClicked_; }
	bool			   Viewport::wasTriggered() const { return wasTriggered_; }
	CalyxMath::Vector2 Viewport::GetSize() const { return size_; }
	CalyxMath::Vector2 Viewport::GetPosition() const {
		// ビューポートの位置を取得
		return viewOrigin_;
	}
	ViewportType Viewport::GetType() const { return type_; }
	void		 Viewport::SetCamera(BaseCamera* camera) { camera_ = camera; }

} // namespace CalyxEditor