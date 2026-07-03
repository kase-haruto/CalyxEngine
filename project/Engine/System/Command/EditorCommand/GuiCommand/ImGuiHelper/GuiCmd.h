#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
/* ========================================================================
/*		include space
/* ===================================================================== */

// engine
#include <Engine/System/Command/Interface/ICommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/SetValueCommand/SetValueCommand.h>
#include <Engine/Scene/Reference/SceneObjectReference.h>

// c++
#include <functional>
#include <cmath>
#include <string>
#include <memory>

// external
#include "Engine/Foundation/Math/Matrix4x4.h"

#include <externals/imgui/imgui.h>

namespace CalyxEngine {
	enum class ParamFilterSection;
}

// math
namespace CalyxEngine {
	struct Vector3;
	struct Vector4;
	struct Vector2;
	CalyxEngine::Matrix4x4 MakeOrthographicMatrixLH(float left,float right,float bottom,float top,float nearZ,
								 float farZ);
} // namespace CalyxEngine

/* ========================================================================
/*		imgui コマンドラッパ
/* ===================================================================== */
namespace GuiCmd{

	//===================================================================*/
	//		dragInt
	//===================================================================*/
	CALYX_API bool DragInt(const char* label, int& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);

	//===================================================================*/
	//		dragFloat
	//===================================================================*/
	CALYX_API bool DragFloat(const char* label, float& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
	CALYX_API bool DragFloat2(const char* label, CalyxEngine::Vector2& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
	CALYX_API bool DragFloat3(const char* label,CalyxEngine::Vector3& value,float speed = 0.01f,float min = 0.0f,float max = 0.0f);
	CALYX_API bool DragFloat4(const char* label, CalyxEngine::Vector4& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
	CALYX_API bool ColoredDragFloat3(const char* label,
						   CalyxEngine::Vector3& value,
						   float speed = 0.1f,
						   float min = 0.0f,
						   float max = 0.0f,
						   const char* format = "%.3f",
						   const char* suffix = "");

	//===================================================================*/
	//		sliderFloat
	//===================================================================*/
	CALYX_API bool SliderFloat(const char* label, float& value, float min = 0.0f, float max = 1.0f);
	CALYX_API bool SliderFloat2(const char* label, CalyxEngine::Vector2& value, float min = 0.0f, float max = 1.0f);
	CALYX_API bool SliderFloat3(const char* label, CalyxEngine::Vector3& value, float min = 0.0f, float max = 1.0f);
	CALYX_API bool SliderFloat4(const char* label, CalyxEngine::Vector4& value, float min = 0.0f, float max = 1.0f);

	//===================================================================*/
	//		colorEdit
	//===================================================================*/
	CALYX_API bool ColorEdit4(const char* label, CalyxEngine::Vector4& value, ImGuiColorEditFlags flags = 0);

	//===================================================================*/
	//		combo
	//===================================================================*/
	CALYX_API bool Combo(const char* label, int& current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);

	//===================================================================*/
	//		checkbox
	//===================================================================*/
	CALYX_API bool CheckBox(const char* label, bool& value);
	
	//===================================================================*/
	//		CollapsingHeader
	//===================================================================*/
	CALYX_API bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);

	//===================================================================*/
	//		Layout Helpers
	//===================================================================*/
	CALYX_API void BeginTableLayout(const char* id = "InspectorTable");
	CALYX_API void EndTableLayout();
	CALYX_API void PropertyText(const char* label, const char* fmt, ...);
	CALYX_API bool SceneObjectReferenceField(const char* label, CalyxEngine::ISceneObjectReference& reference);

	//===================================================================*/
	//		Section Filter Helpers (For Tab View)
	//===================================================================*/
	CALYX_API void SetSectionFilter(CalyxEngine::ParamFilterSection sectionType);
	
	CALYX_API bool BeginSection(CalyxEngine::ParamFilterSection sectionType);
	
	// Ends the current section.
	CALYX_API void EndSection();

}
