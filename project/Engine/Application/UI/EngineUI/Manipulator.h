#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

#include <Engine/Application/UI/EngineUI/IOnViewportTool.h>
#include <Engine/System/Command/EditorCommand/GuizmoCommand/ScopedGizmoCommand.h>

#include <externals/imgui/ImGuizmo.h>
#include <externals/imgui/imgui.h>


class WorldTransform;
class BaseCamera;
struct CalyxMath::Matrix4x4;

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * Manipulator
	 * - トランスフォームマニピュレータクラス
	 * - ImGuizmoを使用した移動・回転・スケールのギズモ操作を提供
	 *---------------------------------------------------------------------------------------*/
	class Manipulator
		: public BaseOnViewportTool {
	public:
		//===================================================================*/
		//					public methods
		//===================================================================*/
		Manipulator();
		void Update();

		/// <summary>
		/// オーバーレイ描画
		/// </summary>
		/// <param name="basePos"></param>
		void RenderOverlay(const ImVec2& basePos) override;

		/// <summary>
		/// ツールバー描画
		/// </summary>
		void RenderToolbar() override;

		//--------- accessor -----------------------------------------------------
		void SetTarget(WorldTransform* target);
		void SetCamera(BaseCamera* camera);
		void SetViewRect(const ImVec2& origin, const ImVec2& size);

	private:
		//===================================================================*/
		//					private methods
		//===================================================================*/

		/// <summary>
		/// 行列計算
		/// </summary>
		/// <param name="m"></param>
		/// <param name="out"></param>
		void				 RowToColumnArray(const CalyxMath::Matrix4x4& m, float out[16]);
		CalyxMath::Matrix4x4 ColumnArrayToRow(const float in_[16]);

		/// <summary>
		/// ImGuizmo による操作・描画処理
		/// </summary>
		void Manipulate();

	private:
		ImGuizmo::OPERATION operation_ = ImGuizmo::TRANSLATE;
		ImGuizmo::MODE		mode_	   = ImGuizmo::WORLD;

		bool								wasUsing = false;
		std::unique_ptr<ScopedGizmoCommand> scopedCmd;

		WorldTransform* target_ = nullptr;
		BaseCamera*		camera_ = nullptr;

		ImVec2 viewOrigin_ = {0, 0};
		ImVec2 viewSize_   = {0, 0};

	private:
		// アイコン
		struct Icon {
			ImTextureID texture = nullptr;
			ImVec2		size{24.0f, 24.0f};
		};

	public:
		Icon iconTranslate_;
		Icon iconRotate_;
		Icon iconScale_;
		Icon iconUniversal_;
		Icon iconWorld_;

		Icon iconDrawGrid_;
	};

} // namespace CalyxEditor