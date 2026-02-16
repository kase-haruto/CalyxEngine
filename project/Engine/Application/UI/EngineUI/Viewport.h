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

// c++
#include <memory>
#include <string>

// externals
#include "Engine/Foundation/Math/Vector3.h"

#include <externals/imgui/imgui.h>

// forward declaration
namespace CalyxMath {
	struct Vector2;
}
class BaseCamera;
class SceneObject;

namespace CalyxEditor {
	class PickingPass;

	/*-----------------------------------------------------------------------------------------
	 * Viewport
	 * - エディタビューポートクラス
	 * - シーンプレビュー用のImGuiウィンドウとカメラ連携を担当
	 *---------------------------------------------------------------------------------------*/
	class Viewport
		: public IEngineUI {
	public:
		//===================================================================*/
		//                    methods
		//===================================================================*/
		Viewport(ViewportType type, const std::string& windowName);

		void Update();						 //< ビューポートの更新処理
		CalyxMath::Vector3 CalculateSpawnPosForPlace(const ImVec2& imagePos);
		void Render(const ImTextureID& tex); //< ImGui上への描画処理
		void Render() {}

		void AddTool(IOnViewportTool* tool); //< ビューポートツールの追加

		//--------- accessor -----------------------------------------------------
		bool			   IsHovered() const;
		bool			   IsClicked() const;
		bool			   wasTriggered() const;
		CalyxMath::Vector2 GetSize() const;
		CalyxMath::Vector2 GetPosition() const; //< ビューポートの位置
		ViewportType	   GetType() const;
		void			   SetCamera(BaseCamera* camera);
		void			   SetPickingPass(PickingPass* pickingPass) { pickingPass_ = pickingPass; }

	private:
		ImVec2 CalcToolPosition(IOnViewportTool* tool,
								const ImVec2&	 viewportPos,
								const ImVec2&	 viewportSize);

	private:
		std::vector<IOnViewportTool*> tools_;
		ViewportType				  type_ = ViewportType::VIEWPORT_NONE; //< ビューポートの種類
		std::string					  windowName_;						   //< ビューポートのウィンドウ名

		ImTextureID textureID_ = nullptr;

		BaseCamera*		   camera_ = nullptr; //< ビューポートに関連付けられたカメラ
		CalyxMath::Vector2 size_{};
		CalyxMath::Vector2 viewOrigin_; //< ImGui上での描画開始位置
		bool			   isHovered_	 = false;
		bool			   isClicked_	 = false;
		bool			   wasTriggered_ = false;

		std::shared_ptr<SceneObject> ghost_		  = nullptr;
		PickingPass*				 pickingPass_ = nullptr;
	};

} // namespace CalyxEditor