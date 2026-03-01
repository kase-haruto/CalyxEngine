#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Viewport.h"
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Editor/PickingPass.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Physics/Ray/RayDetail.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <externals/imgui/ImGuizmo.h>
#include <externals/imgui/imgui.h>

#include <cmath>

namespace CalyxEditor {

namespace {
    inline bool IsPointInRect(const CalyxMath::Vector2& p, const CalyxMath::Vector2& size) {
        return (p.x >= 0.0f && p.y >= 0.0f && p.x < size.x && p.y < size.y);
    }

    inline const PlaceToolPanel::PlaceItem* ReadPlaceItemFromPayload(const ImGuiPayload* payload) {
        if(!payload || !payload->Data || payload->DataSize != (int)sizeof(const PlaceToolPanel::PlaceItem*)) {
            return nullptr;
        }
        return *(const PlaceToolPanel::PlaceItem* const*)payload->Data;
    }
}

Viewport::Viewport(ViewportType type, const std::string& windowName)
    : IEngineUI(windowName), type_(type), windowName_(windowName) {}

void Viewport::Update() {}

CalyxMath::Vector3 Viewport::CalculateSpawnPosForPlace(const ImVec2& imagePos) {
    // マウス位置（Viewportローカル）
    const ImVec2 mousePos = ImGui::GetMousePos();
    const CalyxMath::Vector2 localMouse(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

    // レイ生成
    const ::Ray ray = ::Raycastor::ConvertMouseToRay(
        localMouse,
        camera_->GetViewMatrix(),
        camera_->GetProjectionMatrix(),
        size_);

    // デフォルト：カメラ前方 10（ここが「10奥」）
    CalyxMath::Vector3 spawnPos = ray.origin + ray.direction * 10.0f;

    // ---- GPU picking（成功したら上書き）----
    SceneContext* ctx = SceneContext::Current();
    if(ctx && pickingPass_ && IsPointInRect(localMouse, size_)) {
        const float scaleX = (float)pickingPass_->GetWidth()  / size_.x;
        const float scaleY = (float)pickingPass_->GetHeight() / size_.y;

        const int32_t px = (int32_t)(localMouse.x * scaleX);
        const int32_t py = (int32_t)(localMouse.y * scaleY);

        const uint32_t objID = pickingPass_->GetObjectID(px, py);
        const float    depth = pickingPass_->GetDepth(px, py);

        if(objID > 0 && depth > 0.001f && depth < 0.999f) {
            const float ndcX = (localMouse.x / size_.x) * 2.0f - 1.0f;
            const float ndcY = 1.0f - (localMouse.y / size_.y) * 2.0f;

            const CalyxMath::Vector4 ndcPos(ndcX, ndcY, depth, 1.0f);
            const CalyxMath::Matrix4x4 invVP = CalyxMath::Matrix4x4::Inverse(camera_->GetViewProjectionMatrix());
            const CalyxMath::Vector4 worldH  = invVP * ndcPos;

            if(std::fabs(worldH.w) > 1e-5f) {
                const CalyxMath::Vector3 worldPos = (worldH / worldH.w).xyz();
                spawnPos = worldPos;
            }
        }
    }

    return spawnPos;
}

void Viewport::Render(const ImTextureID& tex) {

    // ---------------- Camera resolve ----------------
    switch(type_) {
    case ViewportType::VIEWPORT_MAIN: {
        auto* mainCam = CameraManager::GetMain3d();
        if(mainCam && camera_ != mainCam) camera_ = mainCam;
        break;
    }
    case ViewportType::VIEWPORT_DEBUG:
    case ViewportType::VIEWPORT_PICKING: {
        auto* debugCam = CameraManager::GetDebug();
        if(debugCam && camera_ != debugCam) camera_ = debugCam;
        break;
    }
    }

    if(!camera_) return;

    textureID_ = tex;
    bool open = true;

    // Begin が false でも End は必須
    if(!ImGui::Begin(windowName_.c_str(), &open,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::End();
        if(!open) SetShow(false);
        return;
    }

    // ---------------- viewport size ----------------
    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    size_ = CalyxMath::Vector2(contentSize.x, contentSize.y);

    const ImVec2 imagePos = ImGui::GetCursorScreenPos();
    viewOrigin_ = CalyxMath::Vector2(imagePos.x, imagePos.y);

    if(size_.y > 0.0f && type_ != ViewportType::VIEWPORT_PICKING) {
        camera_->SetAspectRatio(size_.x / size_.y);
        camera_->UpdateMatrix();
        CameraManager::SetViewportSizeStatic(type_, size_);
    }

    // ---------------- draw image ----------------
    ImGui::SetCursorScreenPos(imagePos);
    ImGui::Image(textureID_, contentSize);

    // 画像矩形（ホバー判定用）
    const ImVec2 imageMin = imagePos;
    const ImVec2 imageMax = ImVec2(imagePos.x + contentSize.x, imagePos.y + contentSize.y);
    const bool   hoverImageRect = ImGui::IsMouseHoveringRect(imageMin, imageMax, false);

    // ============================================================
    // Drop target は Image の直後に固定（条件分岐の奥に入れない）
    // ============================================================
    if(type_ == ViewportType::VIEWPORT_DEBUG) {
        if(ImGui::BeginDragDropTarget()) {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PLACE_ITEM")) {
                const PlaceToolPanel::PlaceItem* item = ReadPlaceItemFromPayload(payload);
                if(item && hoverImageRect) {
                    const CalyxMath::Vector3 spawnPos = CalculateSpawnPosForPlace(imagePos);
                    item->createFunc(spawnPos);

                    if(ghost_) {
                        SceneContext::Current()->RemoveObject(ghost_);
                        ghost_ = nullptr;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    // ============================================================
    // Ghost update（ドラッグ中＋画像矩形内のみ）
    // ============================================================
    if(type_ == ViewportType::VIEWPORT_DEBUG) {

        const ImGuiPayload* dragPayload = ImGui::GetDragDropPayload();
        const bool draggingPlace = (dragPayload && dragPayload->IsDataType("DND_PLACE_ITEM"));

        if(draggingPlace && hoverImageRect) {

            const PlaceToolPanel::PlaceItem* item = ReadPlaceItemFromPayload(dragPayload);
            if(item && item->ghostFactory) {

                const CalyxMath::Vector3 spawnPos = CalculateSpawnPosForPlace(imagePos);

                if(!ghost_) {
                    ghost_ = item->ghostFactory();
                    if(auto go = std::dynamic_pointer_cast<BaseGameObject>(ghost_)) {
                        go->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
                    }
                }

                if(ghost_) {
                    ghost_->GetWorldTransform().translation = spawnPos;
                }
            }
        } else {
            if(ghost_) {
                SceneContext::Current()->RemoveObject(ghost_);
                ghost_ = nullptr;
            }
        }
    }

    // ---------------- Overlay tools / gizmo ----------------
    if(type_ == ViewportType::VIEWPORT_DEBUG) {
        ImGuizmo::SetRect(imagePos.x, imagePos.y, size_.x, size_.y);
        ImGuizmo::SetDrawlist();

        ImGui::BeginGroup();
        for(auto* tool : tools_) {
            auto* base = dynamic_cast<BaseOnViewportTool*>(tool);
            if(!base) continue;

            const ImVec2 viewSize(size_.x, size_.y);
            const ImVec2 pos = base->CalcScreenPosition(imagePos, viewSize);
            tool->RenderOverlay(pos);
        }
        ImGui::EndGroup();
    }

    isHovered_ = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    isClicked_ = ImGui::IsWindowFocused() &&
                 ::CalyxFoundation::Input::GetInstance()->TriggerMouseButton(::CalyxFoundation::MouseButton::Left);

    ImGui::End();

    if(!open) SetShow(false);
}

ImVec2 Viewport::CalcToolPosition(IOnViewportTool* tool, const ImVec2& viewportPos, const ImVec2& viewportSize) {
    ImVec2 basePos;

    OverlayAlign align = OverlayAlign::TopLeft;
    if(auto* base = dynamic_cast<BaseOnViewportTool*>(tool)) {
        align = base->GetOverlayAlign();
    }

    switch(align) {
    case OverlayAlign::TopLeft:     basePos = viewportPos; break;
    case OverlayAlign::TopRight:    basePos = ImVec2(viewportPos.x + viewportSize.x, viewportPos.y); break;
    case OverlayAlign::BottomLeft:  basePos = ImVec2(viewportPos.x, viewportPos.y + viewportSize.y); break;
    case OverlayAlign::BottomRight: basePos = ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y); break;
    case OverlayAlign::CenterTop:   basePos = ImVec2(viewportPos.x + viewportSize.x * 0.5f, viewportPos.y); break;
    }

    return ImVec2(basePos.x + tool->GetOverlayOffset().x,
                  basePos.y + tool->GetOverlayOffset().y);
}

void Viewport::AddTool(IOnViewportTool* tool) { tools_.push_back(tool); }

bool Viewport::IsHovered() const { return isHovered_; }
bool Viewport::IsClicked() const { return isClicked_; }
bool Viewport::wasTriggered() const { return wasTriggered_; }
CalyxMath::Vector2 Viewport::GetSize() const { return size_; }
CalyxMath::Vector2 Viewport::GetPosition() const { return viewOrigin_; }
ViewportType Viewport::GetType() const { return type_; }
void Viewport::SetCamera(BaseCamera* camera) { camera_ = camera; }

} // namespace CalyxEditor
