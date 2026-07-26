#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Graphics/Camera/Viewport/ViewportDetail.h>

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

// c++
#include <memory>
#include <string>
#include <vector>

// externals
#include "Engine/Foundation/Math/Vector3.h"

#include <externals/imgui/imgui.h>

// forward declaration
namespace CalyxEngine {
	struct Vector2;
}
class BaseCamera;
class SceneObject;
struct AssetDragPayload;

namespace CalyxEngine {
	class PickingPass;

	/*-----------------------------------------------------------------------------------------
	 * Viewport
	 * - エディタビューポートクラス
	 * - シーンプレビュー用のImGuiウィンドウとカメラ連携を担当
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief Viewportの機能を提供するクラスです。
	 */
	class Viewport
		: public IEngineUI {
	public:
		//===================================================================*/
		//                    methods
		//===================================================================*/
		Viewport(ViewportType type, const std::string& windowName);

		void Update();						 //< ビューポートの更新処理
		CalyxEngine::Vector3 CalculateSpawnPosForPlace(const ImVec2& imagePos);
		void Render(const ImTextureID& tex); //< ImGui上への描画処理
		void Render() {}

		void AddTool(IOnViewportTool* tool); //< ビューポートツールの追加

		//--------- accessor -----------------------------------------------------
		bool			   IsHovered() const;
		bool			   IsClicked() const;
		bool			   wasTriggered() const;
		CalyxEngine::Vector2 GetSize() const;
		CalyxEngine::Vector2 GetPosition() const; //< ビューポートの位置
		ViewportType	   GetType() const;
		void			   SetCamera(BaseCamera* camera);
		void			   SetPickingPass(PickingPass* pickingPass) { pickingPass_ = pickingPass; }
		void			   SetOverlayToolsEnabled(bool enabled) { overlayToolsEnabled_ = enabled; }
		void			   Set2DPlacementCanvasEnabled(bool enabled);

	private:
		ImVec2 CalcToolPosition(IOnViewportTool* tool,
								const ImVec2&	 viewportPos,
								const ImVec2&	 viewportSize);
		std::shared_ptr<SceneObject> PickObjectAtLocalPoint(const CalyxEngine::Vector2& localPoint) const;
		bool ApplyAssetToObjectAtLocalPoint(const AssetDragPayload& payload, const CalyxEngine::Vector2& localPoint);
		void ClearGhosts();

	private:
		enum class GhostKind {
			None,
			PlaceItem,
			ModelAsset,
			PrefabAsset,
		};

		std::vector<IOnViewportTool*> tools_;
		ViewportType				  type_ = ViewportType::VIEWPORT_NONE; //< ビューポートの種類
		std::string					  windowName_;						   //< ビューポートのウィンドウ名

		ImTextureID textureID_ = nullptr;

		BaseCamera*		   camera_ = nullptr; //< ビューポートに関連付けられたカメラ
		CalyxEngine::Vector2 size_{};
		CalyxEngine::Vector2 viewOrigin_; //< ImGui上での描画開始位置
		bool			   isHovered_	 = false;
		bool			   isClicked_	 = false;
		bool			   wasTriggered_ = false;
		bool			   overlayToolsEnabled_ = true;
		bool			   placementCanvas2DEnabled_ = false;
		float			   placementCanvasZoom_ = 1.0f;
		ImVec2			   placementCanvasPan_{0.0f, 0.0f};

		std::shared_ptr<SceneObject> ghost_		  = nullptr;
		std::vector<std::shared_ptr<SceneObject>> prefabGhosts_;
		GhostKind					 ghostKind_	  = GhostKind::None;
		Guid						 ghostAssetGuid_ = Guid::Empty();
		PickingPass*				 pickingPass_ = nullptr;
	};

} // namespace CalyxEngine
