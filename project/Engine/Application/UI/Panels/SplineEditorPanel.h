#pragma once
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Application/UI/EngineUI/Manipulator.h>
#include <Engine/Objects/Transform/Transform.h>

#include <string>

struct Ray;
struct CalyxMath::Vector3;

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * SplineEditorPanel
	 * - スプラインエディタパネルクラス
	 * - スプラインの制御点編集・プレビュー描画を提供
	 *---------------------------------------------------------------------------------------*/
	class SplineEditorPanel
		: public IEngineUI {
	public:
		SplineEditorPanel() : IEngineUI("SplineEditor") {
			IEngineUI::SetShow(false);
			gizmoTf_.Initialize();
		}
		~SplineEditorPanel() override = default;

		void Render() override;

		void SyncViewportRect(const CalyxMath::Vector2& pos, const CalyxMath::Vector2& size) {
			vpPos_	= pos;
			vpSize_ = size;
		}

		SplineData& Data() { return data_; }
		int			GetSelectedIndex() const { return selectedPoint_; }
		void		SetSelectedIndex(int i) { selectedPoint_ = i; }

	private:
		void DrawToolbar();
		void DrawPointsList();
		void DrawPreviewXZ();

		void HandleGizmoUpdateAndDraw3D();
		Ray	 MakeMouseRay() const;
		int	 PickPointByRayAABB(const Ray& ray, float halfSize, float& outT) const;
		bool IntersectPlane(const Ray& ray, const CalyxMath::Vector3& n, float d, CalyxMath::Vector3& out) const;

	private:
		SplineData data_;
		int		   selectedPoint_ = -1;

		std::string currentPath_;
		bool		gizmoEnabled_ = true;

		CalyxMath::Vector2 vpPos_{0, 0};
		CalyxMath::Vector2 vpSize_{1280, 720};

		bool			   dragging_ = false;
		CalyxMath::Vector3 dragPlaneN_{0, 1, 0};
		float			   dragPlaneD_ = 0.0f;

		std::unique_ptr<Manipulator> manipulator_;
		WorldTransform				 gizmoTf_;
	};
} // namespace CalyxEditor