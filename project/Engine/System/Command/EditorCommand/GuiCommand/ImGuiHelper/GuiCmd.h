#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */

// engine
#include <Engine/System/Command/Interface/ICommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/SetValueCommand/SetValueCommand.h>

// c++
#include <functional>
#include <cmath>
#include <string>
#include <memory>

// external
#include <externals/imgui/imgui.h>

namespace CalyxEditor {
	enum class ParamFilterSection;
}

// math
namespace CalyxMath {
	struct Vector3;
	struct Vector4;
	struct Vector2;
} // namespace CalyxMath

/* ========================================================================
/*		imgui コマンドラッパ
/* ===================================================================== */
namespace GuiCmd{

	//===================================================================*/
	//		dragInt
	//===================================================================*/
	bool DragInt(const char* label, int& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);

	//===================================================================*/
	//		dragFloat
	//===================================================================*/
	bool DragFloat(const char* label, float& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
	bool DragFloat2(const char* label, CalyxMath::Vector2& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
	bool DragFloat3(const char* label,CalyxMath::Vector3& value,float speed = 0.01f,float min = 0.0f,float max = 0.0f);
	bool DragFloat4(const char* label, CalyxMath::Vector4& value, float speed = 0.01f, float min = 0.0f, float max = 0.0f);
	bool ColoredDragFloat3(const char* label,
						   CalyxMath::Vector3& value,
						   float speed = 0.1f,
						   float min = 0.0f,
						   float max = 0.0f,
						   const char* format = "%.3f",
						   const char* suffix = "");

	//===================================================================*/
	//		sliderFloat
	//===================================================================*/
	bool SliderFloat(const char* label, float& value, float min = 0.0f, float max = 1.0f);
	bool SliderFloat2(const char* label, CalyxMath::Vector2& value, float min = 0.0f, float max = 1.0f);
	bool SliderFloat3(const char* label, CalyxMath::Vector3& value, float min = 0.0f, float max = 1.0f);
	bool SliderFloat4(const char* label, CalyxMath::Vector4& value, float min = 0.0f, float max = 1.0f);

	//===================================================================*/
	//		colorEdit
	//===================================================================*/
	bool ColorEdit4(const char* label, CalyxMath::Vector4& value, ImGuiColorEditFlags flags = 0);

	//===================================================================*/
	//		combo
	//===================================================================*/
	bool Combo(const char* label, int& current_item, const char* const items[], int items_count, int popup_max_height_in_items = -1);

	//===================================================================*/
	//		checkbox
	//===================================================================*/
	bool CheckBox(const char* label, bool& value);
	
	//===================================================================*/
	//		CollapsingHeader
	//===================================================================*/
	bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags = 0);

	//===================================================================*/
	//		Layout Helpers
	//===================================================================*/
	void BeginTableLayout(const char* id = "InspectorTable");
	void EndTableLayout();
	void PropertyText(const char* label, const char* fmt, ...);

	//===================================================================*/
	//		Section Filter Helpers (For Tab View)
	//===================================================================*/
	void SetSectionFilter(CalyxEditor::ParamFilterSection sectionType);
	
	bool BeginSection(CalyxEditor::ParamFilterSection sectionType);
	
	// Ends the current section.
	void EndSection();

}