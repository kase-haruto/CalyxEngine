#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "Viewport.h"
#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Application/Effects/FxSystem.h>
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Object/ParticleSystemObject.h>
#include <Engine/Application/Settings/EngineSettings.h>
#include <Engine/Application/System/Environment.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/System/AssetDragPayload.h>
#include <Engine/Assets/System/AssetType.h>
#include <Engine/Application/UI/Panels/PlaceToolPanel.h>
#include <Engine/Editor/PickingPass.h>
#include <Engine/Foundation/Input/Input.h>
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Graphics/Camera/Base/BaseCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/3D/Actor/Library/SceneObjectLibrary.h>
#include <Engine/Physics/Ray/RayDetail.h>
#include <Engine/Physics/Ray/Raycastor.h>
#include <Engine/Scene/Context/SceneContext.h>
#include <Engine/Scene/Utility/SceneUtility.h>
#include <Engine/System/Command/EditorCommand/LevelEditorCommand/CreateObjectCommand/CreateObjectCommand.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <externals/imgui/ImGuizmo.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace CalyxEngine {

namespace {
    constexpr float kPlacementCanvasMinZoom = 0.15f;
    constexpr float kPlacementCanvasMaxZoom = 4.0f;

    inline bool IsPointInRect(const CalyxEngine::Vector2& p, const CalyxEngine::Vector2& size) {
        return (p.x >= 0.0f && p.y >= 0.0f && p.x < size.x && p.y < size.y);
    }

    inline const PlaceToolPanel::PlaceItem* ReadPlaceItemFromPayload(const ImGuiPayload* payload) {
        if(!payload || !payload->Data || payload->DataSize != (int)sizeof(const PlaceToolPanel::PlaceItem*)) {
            return nullptr;
        }
        return *(const PlaceToolPanel::PlaceItem* const*)payload->Data;
    }

    inline const AssetDragPayload* ReadAssetPayload(const ImGuiPayload* payload) {
        if(!payload || !payload->Data || payload->DataSize != (int)sizeof(AssetDragPayload)) {
            return nullptr;
        }
        return reinterpret_cast<const AssetDragPayload*>(payload->Data);
    }

    inline const AssetRecord* GetDraggedModelRecord(const ImGuiPayload* payload) {
        const AssetDragPayload* assetPayload = ReadAssetPayload(payload);
        if(!assetPayload || assetPayload->type != AssetType::Model) {
            return nullptr;
        }

        const AssetRecord* record = AssetDatabase::GetInstance()->Get(assetPayload->guid);
        if(!record || record->type != AssetType::Model) {
            return nullptr;
        }
        return record;
    }

    inline const AssetRecord* GetDraggedPrefabRecord(const ImGuiPayload* payload) {
        const AssetDragPayload* assetPayload = ReadAssetPayload(payload);
        if(!assetPayload || assetPayload->type != AssetType::Prefab) {
            return nullptr;
        }

        const AssetRecord* record = AssetDatabase::GetInstance()->Get(assetPayload->guid);
        if(!record || record->type != AssetType::Prefab) {
            return nullptr;
        }
        return record;
    }

    inline float SnapAxis(float value, float step) {
        const float absStep = std::fabs(step);
        if(absStep <= 1e-5f) {
            return value;
        }
        return std::round(value / absStep) * absStep;
    }

    inline CalyxEngine::Vector3 ApplyPlacementSnap(const CalyxEngine::Vector3& pos) {
        const ManipulatorSettings& settings = EngineSettings::GetInstance()->GetData().manipulator;
        if(!settings.useSnap) {
            return pos;
        }

        return {
            SnapAxis(pos.x, settings.snapTranslate[0]),
            SnapAxis(pos.y, settings.snapTranslate[1]),
            SnapAxis(pos.z, settings.snapTranslate[2]),
        };
    }

    void DrawPlacementCanvasGrid(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, const ImVec2& origin, float scale) {
        drawList->AddRectFilled(min, max, IM_COL32(24, 24, 26, 255));

        const float gridStep = (std::max)(8.0f, 64.0f * scale);
        float startX = min.x + std::fmod(origin.x - min.x, gridStep);
        float startY = min.y + std::fmod(origin.y - min.y, gridStep);
        if(startX < min.x) startX += gridStep;
        if(startY < min.y) startY += gridStep;

        for(float x = startX; x < max.x; x += gridStep) {
            drawList->AddLine({x, min.y}, {x, max.y}, IM_COL32(44, 44, 48, 255), 1.0f);
        }
        for(float y = startY; y < max.y; y += gridStep) {
            drawList->AddLine({min.x, y}, {max.x, y}, IM_COL32(44, 44, 48, 255), 1.0f);
        }
    }

    inline std::shared_ptr<BaseGameObject> CreateModelObjectFromAsset(
        const AssetRecord& record,
        const CalyxEngine::Vector3& pos,
        bool isGhost) {
        const std::string modelName = record.sourcePath.filename().string();
        const std::string objectName = record.sourcePath.stem().string();

        auto obj = SceneAPI::Instantiate<BaseGameObject>(modelName, objectName);
        obj->Initialize();
        if(auto* collider = obj->GetCollider()) {
            collider->SetCollisionEnabled(false);
        }
        obj->GetWorldTransform().translation = pos;

        if(isGhost) {
            obj->SetTransient(true);
            obj->SetBlendMode(BlendMode::ALPHA);
            obj->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
        }

        return obj;
    }

    void AddPrefabObjectsToCurrentScene(
        const std::vector<std::shared_ptr<SceneObject>>& objects,
        const Guid& prefabAssetGuid,
        const CalyxEngine::Vector3& spawnOffset) {
        SceneContext* ctx = SceneContext::Current();
        if(!ctx) return;

        std::unordered_set<SceneObject*> loaded;
        loaded.reserve(objects.size());
        for(const auto& sp : objects) {
            if(sp) loaded.insert(sp.get());
        }

        for(const auto& sp : objects) {
            if(!sp) continue;
            if(prefabAssetGuid.isValid() && !sp->GetPrefabAssetGuid().isValid()) {
                sp->SetPrefabLink(prefabAssetGuid, sp->GetGuid());
            }

            auto parent = sp->GetParent();
            if(!parent || !loaded.contains(parent.get())) {
                sp->GetWorldTransform().translation =
                    sp->GetWorldTransform().translation + spawnOffset;
            }

            ctx->AddObject(sp);
        }
    }

    void AddPrefabGhostObjectsToCurrentScene(
        const std::vector<std::shared_ptr<SceneObject>>& objects,
        const CalyxEngine::Vector3& spawnOffset) {
        SceneContext* ctx = SceneContext::Current();
        if(!ctx) return;

        std::unordered_set<SceneObject*> loaded;
        loaded.reserve(objects.size());
        for(const auto& sp : objects) {
            if(sp) loaded.insert(sp.get());
        }

        for(const auto& sp : objects) {
            if(!sp) continue;
            sp->SetTransient(true);
            sp->SetEnablePicking(false);
            if(auto go = std::dynamic_pointer_cast<BaseGameObject>(sp)) {
                go->SetBlendMode(BlendMode::ALPHA);
                go->SetColor({0.35f, 0.65f, 1.0f, 0.45f});
            }

            auto parent = sp->GetParent();
            if(!parent || !loaded.contains(parent.get())) {
                sp->GetWorldTransform().translation =
                    sp->GetWorldTransform().translation + spawnOffset;
            }

            ctx->AddObject(sp);
        }
    }
}

Viewport::Viewport(ViewportType type, const std::string& windowName)
    : IEngineUI(windowName), type_(type), windowName_(windowName) {}

void Viewport::Update() {}

void Viewport::ClearGhosts() {
    if(auto* ctx = SceneContext::Current()) {
        if(ghost_) {
            ctx->RemoveObject(ghost_);
        }
        for(auto& prefabGhost : prefabGhosts_) {
            if(prefabGhost && ctx->GetObjectLibrary() && ctx->GetObjectLibrary()->Contains(prefabGhost)) {
                ctx->RemoveObject(prefabGhost);
            }
        }
    }
    ghost_ = nullptr;
    prefabGhosts_.clear();
    ghostKind_ = GhostKind::None;
    ghostAssetGuid_ = Guid::Empty();
}

CalyxEngine::Vector3 Viewport::CalculateSpawnPosForPlace(const ImVec2& imagePos) {
    // マウス位置（Viewportローカル）
    const ImVec2 mousePos = ImGui::GetMousePos();
    const CalyxEngine::Vector2 localMouse(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

    // レイ生成
    const ::Ray ray = ::Raycastor::ConvertMouseToRay(
        localMouse,
        camera_->GetViewMatrix(),
        camera_->GetProjectionMatrix(),
        size_);

    // デフォルト：カメラ前方 10（ここが「10奥」）
    CalyxEngine::Vector3 spawnPos = ray.origin + ray.direction * 10.0f;

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

            const CalyxEngine::Vector4 ndcPos(ndcX, ndcY, depth, 1.0f);
            const CalyxEngine::Matrix4x4 invVP = CalyxEngine::Matrix4x4::Inverse(camera_->GetViewProjectionMatrix());
            const CalyxEngine::Vector4 worldH  = invVP * ndcPos;

            if(std::fabs(worldH.w) > 1e-5f) {
                const CalyxEngine::Vector3 worldPos = (worldH / worldH.w).xyz();
                spawnPos = worldPos;
            }
        }
    }

    return ApplyPlacementSnap(spawnPos);
}

std::shared_ptr<SceneObject> Viewport::PickObjectAtLocalPoint(const CalyxEngine::Vector2& localPoint) const {
    SceneContext* ctx = SceneContext::Current();
    if(!ctx || !camera_ || !IsPointInRect(localPoint, size_)) return nullptr;

    if(pickingPass_) {
        const float scaleX = static_cast<float>(pickingPass_->GetWidth()) / size_.x;
        const float scaleY = static_cast<float>(pickingPass_->GetHeight()) / size_.y;

        const int32_t px = static_cast<int32_t>(localPoint.x * scaleX);
        const int32_t py = static_cast<int32_t>(localPoint.y * scaleY);

        const uint32_t objID = pickingPass_->GetObjectID(px, py);
        if(objID > 0) {
            if(auto* library = ctx->GetObjectLibrary()) {
                if(auto sp = library->FindSharedByPickingID(objID)) {
                    return sp;
                }
            }
        }
    }

    auto* library = ctx->GetObjectLibrary();
    if(!library) return nullptr;

    const ::Ray ray = ::Raycastor::ConvertMouseToRay(
        localPoint,
        camera_->GetViewMatrix(),
        camera_->GetProjectionMatrix(),
        size_);

    auto hit = ::Raycastor::Raycast(ray, library->GetAllObjectsRaw());
    if(!hit) return nullptr;

    return ctx->FindSharedObject(static_cast<SceneObject*>(hit->hitObject));
}

bool Viewport::ApplyAssetToObjectAtLocalPoint(const AssetDragPayload& payload, const CalyxEngine::Vector2& localPoint) {
    auto target = PickObjectAtLocalPoint(localPoint);
    if(!target) return false;

    if(payload.type == AssetType::Material) {
        auto gameObject = std::dynamic_pointer_cast<BaseGameObject>(target);
        if(!gameObject || !gameObject->GetModel()) return false;

        const Guid before = gameObject->GetModel()->GetMaterialGuid();
        const Guid after = payload.guid;
        if(before == after) return false;

        auto apply = [gameObject](const Guid& guid) {
            if(gameObject && gameObject->GetModel()) {
                gameObject->GetModel()->SetMaterialGuid(guid);
            }
        };
        CommandManager::GetInstance()->Execute(
            std::make_unique<ValueEditCommand<Guid>>("Apply Material Asset", before, after, apply));
        return true;
    }

    if(payload.type == AssetType::Texture) {
        if(auto gameObject = std::dynamic_pointer_cast<BaseGameObject>(target)) {
            if(!gameObject->GetModel()) return false;

            const Guid before = gameObject->GetModel()->GetTextureGuid();
            const Guid after = payload.guid;
            if(before == after) return false;

            auto apply = [gameObject](const Guid& guid) {
                if(gameObject && gameObject->GetModel()) {
                    gameObject->GetModel()->SetTextureGuid(guid);
                }
            };
            CommandManager::GetInstance()->Execute(
                std::make_unique<ValueEditCommand<Guid>>("Apply Texture Asset", before, after, apply));
            return true;
        }

        if(auto particleObject = std::dynamic_pointer_cast<CalyxEngine::ParticleSystemObject>(target)) {
            auto emitter = particleObject->GetEmitter();
            if(!emitter) return false;

            const Guid before = emitter->GetTextureGuid();
            const Guid after = payload.guid;
            if(before == after) return false;

            auto apply = [emitter](const Guid& guid) {
                if(emitter) {
                    emitter->SetTextureGuid(guid);
                }
            };
            CommandManager::GetInstance()->Execute(
                std::make_unique<ValueEditCommand<Guid>>("Apply Particle Texture Asset", before, after, apply));
            return true;
        }
    }

    return false;
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
    size_ = CalyxEngine::Vector2(contentSize.x, contentSize.y);

    const ImVec2 contentPos = ImGui::GetCursorScreenPos();
    ImVec2 imagePos = contentPos;
    ImVec2 imageSize = contentSize;
    const bool usePlacementCanvas2D =
        placementCanvas2DEnabled_ &&
        type_ == ViewportType::VIEWPORT_MAIN &&
        contentSize.x > 0.0f &&
        contentSize.y > 0.0f;

    if(usePlacementCanvas2D) {
        const ImVec2 contentMin = contentPos;
        const ImVec2 contentMax(contentPos.x + contentSize.x, contentPos.y + contentSize.y);
        const float baseScale = (std::min)(
            contentSize.x / static_cast<float>(kGameWidth),
            contentSize.y / static_cast<float>(kGameHeight));
        const float scale = (std::max)(0.01f, baseScale * placementCanvasZoom_);

        if(ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            const ImGuiIO& io = ImGui::GetIO();
            if(ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f) ||
               (ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f) && !ImGuizmo::IsUsing())) {
                placementCanvasPan_.x += io.MouseDelta.x;
                placementCanvasPan_.y += io.MouseDelta.y;
            }
            if(io.MouseWheel != 0.0f) {
                placementCanvasZoom_ = std::clamp(
                    placementCanvasZoom_ * (1.0f + io.MouseWheel * 0.10f),
                    kPlacementCanvasMinZoom,
                    kPlacementCanvasMaxZoom);
            }
        }

        imageSize = {
            static_cast<float>(kGameWidth) * scale,
            static_cast<float>(kGameHeight) * scale};
        imagePos = {
            contentPos.x + contentSize.x * 0.5f - imageSize.x * 0.5f + placementCanvasPan_.x,
            contentPos.y + contentSize.y * 0.5f - imageSize.y * 0.5f + placementCanvasPan_.y};

        DrawPlacementCanvasGrid(ImGui::GetWindowDrawList(), contentMin, contentMax, imagePos, scale);
        size_ = CalyxEngine::Vector2(imageSize.x, imageSize.y);
    }

    viewOrigin_ = CalyxEngine::Vector2(imagePos.x, imagePos.y);

    if(size_.y > 0.0f && type_ != ViewportType::VIEWPORT_PICKING) {
        const CalyxEngine::Vector2 renderSize = usePlacementCanvas2D
            ? CalyxEngine::Vector2(contentSize.x, contentSize.y)
            : size_;
        camera_->SetAspectRatio(renderSize.x / renderSize.y);
        camera_->UpdateMatrix();
        CameraManager::SetViewportSizeStatic(type_, renderSize);
    }

    // ---------------- draw image ----------------
    ImGui::SetCursorScreenPos(imagePos);
    ImGui::Image(textureID_, imageSize);
    if(usePlacementCanvas2D) {
        ImGui::GetWindowDrawList()->AddRect(
            imagePos,
            {imagePos.x + imageSize.x, imagePos.y + imageSize.y},
            IM_COL32(255, 186, 84, 255),
            0.0f,
            0,
            2.0f);
        ImGui::SetCursorScreenPos(contentPos);
        ImGui::Dummy(contentSize);
    }

    // 画像矩形（ホバー判定用）
    const ImVec2 imageMin = imagePos;
    const ImVec2 imageMax = ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y);
    const bool   hoverImageRect = ImGui::IsMouseHoveringRect(imageMin, imageMax, false);

    // ============================================================
    // Drop target は Image の直後に固定（条件分岐の奥に入れない）
    // ============================================================
    const bool acceptsViewportPlacement =
        overlayToolsEnabled_ &&
        (type_ == ViewportType::VIEWPORT_DEBUG || type_ == ViewportType::VIEWPORT_MAIN);

    if(acceptsViewportPlacement) {
        if(ImGui::BeginDragDropTarget()) {
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_PLACE_ITEM")) {
                const PlaceToolPanel::PlaceItem* item = ReadPlaceItemFromPayload(payload);
                if(item && hoverImageRect && payload->IsDelivery()) {
                    const CalyxEngine::Vector3 spawnPos = CalculateSpawnPosForPlace(imagePos);
                    item->createFunc(spawnPos);

                    ClearGhosts();
                }
            }
            if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CALYX_ASSET")) {
                const AssetDragPayload* assetPayload = ReadAssetPayload(payload);
                if(assetPayload && hoverImageRect && payload->IsDelivery() &&
                   (assetPayload->type == AssetType::Material || assetPayload->type == AssetType::Texture)) {
                    const ImVec2 mousePos = ImGui::GetMousePos();
                    const CalyxEngine::Vector2 localMouse(mousePos.x - imagePos.x, mousePos.y - imagePos.y);
                    ApplyAssetToObjectAtLocalPoint(*assetPayload, localMouse);
                }

                const AssetRecord* record = GetDraggedModelRecord(payload);
                if(record && hoverImageRect && payload->IsDelivery()) {
                    const CalyxEngine::Vector3 spawnPos = CalculateSpawnPosForPlace(imagePos);
                    const AssetGUID guid = record->guid;

                    auto factory = [guid, spawnPos]() {
                        const AssetRecord* freshRecord = AssetDatabase::GetInstance()->Get(guid);
                        if(!freshRecord) {
                            return std::shared_ptr<BaseGameObject>{};
                        }
                        return CreateModelObjectFromAsset(*freshRecord, spawnPos, false);
                    };

                    CommandManager::GetInstance()->Execute(
                        std::make_unique<CreateObjectCommand<BaseGameObject>>(
                            SceneContext::Current(), factory, "Create Model Asset"));

                    ClearGhosts();
                }

                const AssetRecord* prefabRecord = GetDraggedPrefabRecord(payload);
                if(prefabRecord && hoverImageRect && payload->IsDelivery()) {
                    const CalyxEngine::Vector3 spawnPos = CalculateSpawnPosForPlace(imagePos);
                    auto objects = PrefabSerializer::Load(
                        prefabRecord->sourcePath.string(),
                        PrefabSerializer::LoadOptions{false, prefabRecord->guid});
                    AddPrefabObjectsToCurrentScene(objects, prefabRecord->guid, spawnPos);

                    ClearGhosts();
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    // ============================================================
    // Ghost update（ドラッグ中＋画像矩形内のみ）
    // ============================================================
    if(acceptsViewportPlacement) {

        const ImGuiPayload* dragPayload = ImGui::GetDragDropPayload();
        const bool draggingPlace = (dragPayload && dragPayload->IsDataType("DND_PLACE_ITEM"));
        const bool draggingModelAsset = (dragPayload && dragPayload->IsDataType("CALYX_ASSET") &&
                                         GetDraggedModelRecord(dragPayload));
        const bool draggingPrefabAsset = (dragPayload && dragPayload->IsDataType("CALYX_ASSET") &&
                                          GetDraggedPrefabRecord(dragPayload));

        if((draggingPlace || draggingModelAsset || draggingPrefabAsset) && hoverImageRect) {

            const CalyxEngine::Vector3 spawnPos = CalculateSpawnPosForPlace(imagePos);

            if(draggingPlace) {
                const PlaceToolPanel::PlaceItem* item = ReadPlaceItemFromPayload(dragPayload);
                if(item && item->ghostFactory) {

                    if((ghost_ && ghostKind_ != GhostKind::PlaceItem) || !prefabGhosts_.empty()) {
                        ClearGhosts();
                    }

                    if(!ghost_) {
                        ghost_ = item->ghostFactory();
                        ghostKind_ = GhostKind::PlaceItem;
                        ghostAssetGuid_ = Guid::Empty();
                        if(auto go = std::dynamic_pointer_cast<BaseGameObject>(ghost_)) {
                            go->SetColor({1.0f, 1.0f, 1.0f, 0.5f});
                        }
                    }

                    if(ghost_) {
                        ghost_->GetWorldTransform().translation = spawnPos;
                    }
                }
            } else if(draggingModelAsset) {
                const AssetRecord* record = GetDraggedModelRecord(dragPayload);
                if(record) {
                    if((ghost_ && (ghostKind_ != GhostKind::ModelAsset || ghostAssetGuid_ != record->guid)) ||
                       !prefabGhosts_.empty()) {
                        ClearGhosts();
                    }

                    if(!ghost_) {
                        ghost_ = CreateModelObjectFromAsset(*record, spawnPos, true);
                        ghostKind_ = GhostKind::ModelAsset;
                        ghostAssetGuid_ = record->guid;
                    }

                    if(ghost_) {
                        ghost_->GetWorldTransform().translation = spawnPos;
                    }
                }
            } else if(draggingPrefabAsset) {
                const AssetRecord* record = GetDraggedPrefabRecord(dragPayload);
                if(ghost_ || (record && ghostAssetGuid_ != record->guid)) {
                    ClearGhosts();
                }
                if(record) {
                    if(prefabGhosts_.empty()) {
                        prefabGhosts_ = PrefabSerializer::Load(
                            record->sourcePath.string(),
                            PrefabSerializer::LoadOptions{false, Guid::Empty()});
                        AddPrefabGhostObjectsToCurrentScene(prefabGhosts_, spawnPos);
                        ghostKind_ = GhostKind::PrefabAsset;
                        ghostAssetGuid_ = record->guid;
                    } else {
                        std::unordered_set<SceneObject*> loaded;
                        loaded.reserve(prefabGhosts_.size());
                        for(auto& sp : prefabGhosts_) {
                            if(sp) loaded.insert(sp.get());
                        }
                        for(auto& sp : prefabGhosts_) {
                            if(!sp) continue;
                            auto parent = sp->GetParent();
                            if(!parent || !loaded.contains(parent.get())) {
                                sp->GetWorldTransform().translation = spawnPos;
                            }
                        }
                    }
                }
            }
        } else {
            ClearGhosts();
        }
    }

    // ---------------- Overlay tools / gizmo ----------------
    if((type_ == ViewportType::VIEWPORT_DEBUG || type_ == ViewportType::VIEWPORT_MAIN) && overlayToolsEnabled_) {
        ImGuizmo::SetRect(imagePos.x, imagePos.y, size_.x, size_.y);
        ImGuizmo::SetDrawlist();

        ImGui::BeginGroup();
        for(auto* tool : tools_) {
            auto* base = dynamic_cast<BaseOnViewportTool*>(tool);
            if(!base) continue;

            if(auto* manipulator = dynamic_cast<Manipulator*>(tool)) {
                manipulator->SetActiveViewportType(type_);
                manipulator->SetViewRect(imagePos, imageSize);
            }

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
CalyxEngine::Vector2 Viewport::GetSize() const { return size_; }
CalyxEngine::Vector2 Viewport::GetPosition() const { return viewOrigin_; }
ViewportType Viewport::GetType() const { return type_; }
void Viewport::SetCamera(BaseCamera* camera) { camera_ = camera; }

void Viewport::Set2DPlacementCanvasEnabled(bool enabled) {
    if(placementCanvas2DEnabled_ == enabled) return;
    placementCanvas2DEnabled_ = enabled;
    placementCanvasZoom_ = 1.0f;
    placementCanvasPan_ = {0.0f, 0.0f};
}

} // namespace CalyxEngine
