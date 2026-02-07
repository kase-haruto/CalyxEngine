#define IMGUI_DEFINE_MATH_OPERATORS
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmdInternal.h>

// math
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

// external
#include "Engine/Application/UI/Panels/InspectorPanel.h"
#include "Engine/Editor/Collection/EditorCollection.h"

#include <externals/imgui/imgui.h>
#include <externals/imgui/imgui_internal.h>

#include <string>
#include <cstdio>

/* ==========================================================================================================
/*		Internal Helpers (Custom Rendering)
/* ======================================================================================================== */
namespace {
	using namespace CalyxMath;

	// テーブルレイアウトの使用フラグ
	static bool useTableLayout_ = false;

	// Internal helper to begin a property row
	static void BeginPropertyRow(const char* label) {
		if (useTableLayout_) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			
			// ラベル表示（ID部分は隠す）
			const char* label_end = strstr(label, "##");
			if (label_end == nullptr) {
				ImGui::TextUnformatted(label); 
			} else {
				ImGui::TextUnformatted(label, label_end);
			}

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-1.0f); // 右カラムいっぱいに
		} else {
			// 通常モード（必要ならラベル表示など）
			// ImGui::Text("%s", label); // 標準のDrag系はラベルを内部で描写するので不要
		}
	}
	
	// ただしIDの一意性は保つ必要があるため "##Label" 形式にする
	static std::string GetWidgetLabel(const char* label) {
		if (useTableLayout_) {
			// もともと "Name##ID" だった場合 -> "##Name##ID"
			return std::string("##") + label;
		}
		return std::string(label);
	}

	//-------------------------------------------------------------------------
	// フォーマット: [Label] [X] [Value] [Y] [Value] ...
	//-------------------------------------------------------------------------
	bool CustomDragScalarAxis(
    const char* label_id,
    ImGuiDataType data_type,
    void* p_data,
    float v_speed,
    const void* p_min,
    const void* p_max,
    const char* format,
    const ImVec4& color_axis)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label_id);

    const float w = ImGui::CalcItemWidth();
    const ImVec2 label_size = ImGui::CalcTextSize(label_id, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos,
        window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));

    const ImRect total_bb = frame_bb;

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &frame_bb, ImGuiItemFlags_Inputable))
        return false;

    const bool hovered = ImGui::ItemHoverable(frame_bb, id, g.LastItemData.InFlags);
    const bool temp_input_allowed = (g.LastItemData.InFlags & ImGuiItemFlags_Inputable) != 0;
    bool temp_input_is_active = temp_input_allowed && ImGui::TempInputIsActive(id);

    // -------------------------
    //  レイアウト計算（先に全部）
    // -------------------------
    const float axis_label_w = style.FramePadding.y * 2.0f + 5.0f;
    const float bar_width = 3.0f;

    const ImRect axis_line_bb(
        frame_bb.Min,
        ImVec2(frame_bb.Min.x + bar_width, frame_bb.Max.y));

    const ImRect axis_bb(
        frame_bb.Min,
        frame_bb.Min + ImVec2(axis_label_w, frame_bb.GetHeight()));

    const ImRect value_bb(
        frame_bb.Min + ImVec2(axis_label_w, 0),
        frame_bb.Max);

    // -------------------------
    //   ダブルクリック → 直接入力
    // -------------------------
    if (!temp_input_is_active)
    {
        if (hovered && (g.IO.MouseClicked[0] || g.NavActivateId == id))
        {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            ImGui::FocusWindow(window);
        }

        // ★ ダブルクリックされたら入力モードへ移行
        if (hovered && g.IO.MouseDoubleClicked[0])
        {
            ImGui::SetActiveID(id, window);
            ImGui::SetFocusID(id, window);
            temp_input_is_active = true;
        }
    }

    // -------------------------
    //   TempInput (直接入力モード)
    // -------------------------
    if (temp_input_is_active)
    {
        return ImGui::TempInputScalar(
            value_bb,      // 入力エリア（必ず value_bb）
            id,
            label_id,
            data_type,
            p_data,
            format,
            p_min,
            p_max
        );
    }

    // -------------------------
    // ドラッグ挙動
    // -------------------------
    bool modified = ImGui::DragBehavior(
        id, data_type, p_data, v_speed,
        p_min, p_max, format, 0);

    if (modified)
        ImGui::MarkItemEdited(id);

    // -------------------------
    // 描画
    // -------------------------
    ImGui::RenderNavHighlight(frame_bb, id);

    const ImU32 frame_col =
        ImGui::GetColorU32(
            g.ActiveId == id ? ImGuiCol_FrameBgActive :
            hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);

    // メインフレーム
    ImGui::RenderFrame(value_bb.Min, value_bb.Max, frame_col, true, style.FrameRounding);

    // 左のカラーライン（軸色）
    ImGui::RenderFrame(axis_line_bb.Min, axis_line_bb.Max,
        ImGui::GetColorU32(color_axis), false, 0.0f);

    // 軸ラベル（X / Y / Z など）
    const char* letter_display = label_id;
    if (strncmp(label_id, "##", 2) == 0 && strlen(label_id) > 2)
        letter_display += 2;

    if (letter_display[0] != '\0')
    {
        ImGui::PushStyleColor(ImGuiCol_Text, color_axis);
        ImVec2 text_pos = frame_bb.Min +
            ImVec2(bar_width + 3.0f, (frame_bb.GetHeight() - ImGui::GetTextLineHeight()) * 0.5f);
        ImGui::RenderText(text_pos, letter_display);
        ImGui::PopStyleColor();
    }

    // 数値表示
    char value_buf[64];
    const char* value_buf_end =
        value_buf + ImGui::DataTypeFormatString(
            value_buf, IM_ARRAYSIZE(value_buf),
            data_type, p_data, format);

    ImGui::RenderTextClipped(
        value_bb.Min, value_bb.Max,
        value_buf, value_buf_end, NULL,
        ImVec2(0.5f, 0.5f));

    return modified;
}

	//-------------------------------------------------------------------------
	// トグルスイッチ (CheckBoxの代替)
	//-------------------------------------------------------------------------
	bool ToggleButton(const char* str_id, bool* v)
	{
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems) return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(str_id);

		float height = ImGui::GetFrameHeight();
		float width = height * 1.8f;
		float radius = height * 0.50f;

		const ImVec2 label_size = ImGui::CalcTextSize(str_id, NULL, true);
		const ImRect total_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
		
		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id)) return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(total_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !(*v);
			ImGui::MarkItemEdited(id);
		}

		// 描画
		const ImRect check_bb(total_bb.Min, total_bb.Min + ImVec2(width, height));
		
		// 背景 (Pill Shape)
		// ホバー時は少し明るく、アクティブ時はアクセントカラー
		ImVec4 col_bg;
		if (*v) {
			// On Color (Red-Orange Accent)
			col_bg = hovered ? ImVec4(1.00f, 0.45f, 0.25f, 1.00f) : ImVec4(1.00f, 0.35f, 0.15f, 1.00f);
		} else {
			// Off Color (Dark Gray)
			col_bg = hovered ? ImVec4(0.25f, 0.25f, 0.25f, 1.00f) : ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
		}

		ImGui::GetWindowDrawList()->AddRectFilled(check_bb.Min, check_bb.Max, ImGui::GetColorU32(col_bg), height * 0.5f);

		// 円形のノブ
		float knob_pad = 3.0f; // 少し余白を広げる
		float knob_r = (height * 0.5f) - knob_pad;
		// 左位置と右位置
		float knob_x_off = check_bb.Min.x + radius;
		float knob_x_on  = check_bb.Max.x - radius;
		
		float knob_x = (*v) ? knob_x_on : knob_x_off;
		
		ImVec2 knob_center(knob_x, check_bb.GetCenter().y);
		// ノブは白（オフ時は少し暗くしてもよいが白で統一して視認性確保）
		ImVec4 col_knob = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
		
		ImGui::GetWindowDrawList()->AddCircleFilled(knob_center, knob_r, ImGui::GetColorU32(col_knob));

		if (label_size.x > 0.0f)
			ImGui::RenderText(ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y), str_id);

		return pressed;
	}

} // namespace

/* ==========================================================================================================
/*			GuiCmd Implementation
/* ======================================================================================================== */
namespace GuiCmd {

#pragma region DragInt
	bool DragInt(const char* label, int& value, float speed, float min, float max) {
		BeginPropertyRow(label); // Row開始 + ラベル描画（テーブル時）

		static GuiCmdInternal::GuiCmdSetValueComputer<int> computer;
		int temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::DragInt(widgetLabel.c_str(), &temp, speed, (int)min, (int)max);

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const int& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		
		value = temp;
		return changed;
	}
#pragma endregion

#pragma region DragFloat
	bool DragFloat(const char* label, float& value, float speed, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<float> computer;
		float temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::DragFloat(widgetLabel.c_str(), &temp, speed, min, max);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const float& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}

	bool DragFloat2(const char* label, CalyxMath::Vector2& value, float speed, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector2> computer;
		CalyxMath::Vector2 temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		ImGui::PushID(widgetLabel.c_str());
		ImGui::BeginGroup();
		
		float full_width = ImGui::CalcItemWidth();
		
		// テーブルモードならラベル表示しないので幅はそのまま使える
		float item_width = (full_width - ImGui::GetStyle().ItemInnerSpacing.x) / 2.0f;
		
		bool changed = false;
		bool any_activated = false;
		
		ImGui::PushItemWidth(item_width);
		
		// X
		if (CustomDragScalarAxis("X", ImGuiDataType_Float, &temp.x, speed, &min, &max, "%.3f", ImVec4(0.8f, 0.3f, 0.3f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true; 
		
		ImGui::SameLine();
		// Y
		if (CustomDragScalarAxis("Y", ImGuiDataType_Float, &temp.y, speed, &min, &max, "%.3f", ImVec4(0.3f, 0.7f, 0.3f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;
		
		ImGui::PopItemWidth();

		// テーブルモードでないならラベルを描画
		if (!useTableLayout_) {
			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("%s", label);
		}

		ImGui::EndGroup();
		ImGui::PopID();
		
		value = temp;

		if (any_activated) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector2& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		
		return changed;
	}

	// Float3
	bool DragFloat3(const char* label, CalyxMath::Vector3& value, float speed, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector3> computer;
		CalyxMath::Vector3 temp = value;

		std::string widgetLabel = GetWidgetLabel(label);
		ImGui::PushID(widgetLabel.c_str());
		ImGui::BeginGroup();

		float full_width = ImGui::CalcItemWidth();
		float item_width = (full_width - ImGui::GetStyle().ItemInnerSpacing.x * 2.0f) / 3.0f;

		bool changed = false;
		bool any_activated = false;

		ImGui::PushItemWidth(item_width);
		if (CustomDragScalarAxis("X", ImGuiDataType_Float, &temp.x, speed, &min, &max, "%.3f", ImVec4(0.8f, 0.3f, 0.3f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;
		ImGui::SameLine();

		if (CustomDragScalarAxis("Y", ImGuiDataType_Float, &temp.y, speed, &min, &max, "%.3f", ImVec4(0.3f, 0.7f, 0.3f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;
		ImGui::SameLine();

		if (CustomDragScalarAxis("Z", ImGuiDataType_Float, &temp.z, speed, &min, &max, "%.3f", ImVec4(0.3f, 0.3f, 0.9f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;

		ImGui::PopItemWidth();

		if (!useTableLayout_) {
			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("%s", label);
		}

		ImGui::EndGroup();
		ImGui::PopID();

		value = temp;

		if (any_activated) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector3& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}

	// Float4
	bool DragFloat4(const char* label, CalyxMath::Vector4& value, float speed, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector4> computer;
		CalyxMath::Vector4 temp = value;

		std::string widgetLabel = GetWidgetLabel(label);
		ImGui::PushID(widgetLabel.c_str());
		ImGui::BeginGroup();

		float full_width = ImGui::CalcItemWidth();
		float item_width = (full_width - ImGui::GetStyle().ItemInnerSpacing.x * 3.0f) / 4.0f;

		bool changed = false;
		bool any_activated = false;

		ImGui::PushItemWidth(item_width);
		if (CustomDragScalarAxis("X", ImGuiDataType_Float, &temp.x, speed, &min, &max, "%.3f", ImVec4(0.8f, 0.3f, 0.3f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;
		ImGui::SameLine();

		if (CustomDragScalarAxis("Y", ImGuiDataType_Float, &temp.y, speed, &min, &max, "%.3f", ImVec4(0.3f, 0.7f, 0.3f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;
		ImGui::SameLine();

		if (CustomDragScalarAxis("Z", ImGuiDataType_Float, &temp.z, speed, &min, &max, "%.3f", ImVec4(0.3f, 0.3f, 0.9f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;
		ImGui::SameLine();
		
		if (CustomDragScalarAxis("W", ImGuiDataType_Float, &temp.w, speed, &min, &max, "%.3f", ImVec4(0.7f, 0.7f, 0.7f, 1.0f))) changed = true;
		if (ImGui::IsItemActivated()) any_activated = true;

		ImGui::PopItemWidth();

		if (!useTableLayout_) {
			ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
			ImGui::Text("%s", label);
		}

		ImGui::EndGroup();
		ImGui::PopID();

		value = temp;
		if (any_activated) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector4& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}

	bool ColoredDragFloat3(const char* label, CalyxMath::Vector3& value, float speed, float min, float max, const char* , const char* ) {
		return DragFloat3(label, value, speed, min, max); 
	}
#pragma endregion

#pragma region SliderFloat
	//-------------------------------------------------------------------------
	// SliderFloat
	//-------------------------------------------------------------------------
	bool SliderFloat(const char* label, float& value, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<float> computer;
		float temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::SliderFloat(widgetLabel.c_str(), &temp, min, max);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const float& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}

	bool SliderFloat2(const char* label, CalyxMath::Vector2& value, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector2> computer;
		CalyxMath::Vector2 temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::SliderFloat2(widgetLabel.c_str(), &temp.x, min, max);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector2& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}

	bool SliderFloat3(const char* label, CalyxMath::Vector3& value, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector3> computer;
		CalyxMath::Vector3 temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::SliderFloat3(widgetLabel.c_str(), &temp.x, min, max);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector3& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}

	bool SliderFloat4(const char* label, CalyxMath::Vector4& value, float min, float max) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector4> computer;
		CalyxMath::Vector4 temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::SliderFloat4(widgetLabel.c_str(), &temp.x, min, max);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector4& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}
#pragma endregion

#pragma region ColorEdit
	bool ColorEdit4(const char* label, CalyxMath::Vector4& value, ImGuiColorEditFlags flags) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<CalyxMath::Vector4> computer;
		CalyxMath::Vector4 temp = value;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::ColorEdit4(widgetLabel.c_str(), &temp.x, flags);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const CalyxMath::Vector4& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}
#pragma endregion

#pragma region Combo
	bool Combo(const char* label, int& current_item, const char* const items[], int items_count, int popup_max_height_in_items) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<int> computer;
		int temp = current_item;
		
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ImGui::Combo(widgetLabel.c_str(), &temp, items, items_count, popup_max_height_in_items);
		
		// マウスが押された 検知開始
		if (ImGui::IsItemActivated()) computer.Begin(current_item);
		
		current_item = temp;

		// マウスが離れた
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(current_item, [&current_item](const int& v) { current_item = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		
		return changed;
	}
#pragma endregion

#pragma region CheckBox
	bool CheckBox(const char* label, bool& value) {
		BeginPropertyRow(label);

		static GuiCmdInternal::GuiCmdSetValueComputer<bool> computer;
		bool temp = value;
		
		// Use custom ToggleButton
		std::string widgetLabel = GetWidgetLabel(label);
		bool changed = ToggleButton(widgetLabel.c_str(), &temp);
		value = temp;

		if (ImGui::IsItemActivated()) computer.Begin(value);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			std::string labelStr(label);
			auto cmd = computer.End(value, [&value](const bool& v) { value = v; }, labelStr);
			if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
		}
		return changed;
	}
#pragma endregion

#pragma region CollapsingHeader
	bool CollapsingHeader(const char* label, ImGuiTreeNodeFlags flags) {
		// テーブルモード内のヘッダーは、行を占有しつつ背景を描画
		if (useTableLayout_) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn(); 
			
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
			bool isOpen = ImGui::TreeNodeEx(label, flags | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::PopStyleVar();
			
			// ダミーのカラム進行 (TreeNodeExはカラムを消費しない場合もあるが、安全策)
			ImGui::TableNextColumn(); 
			
			return isOpen;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f); // 少し角ばらせる

		// 少し余白を入れる
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		bool isOpen = ImGui::CollapsingHeader(label, flags);

		ImGui::PopStyleVar();

		return isOpen;
	}
#pragma endregion

	//===================================================================*/
	//		Layout Helpers
	//===================================================================*/
	void BeginTableLayout(const char* id) {
		useTableLayout_ = true;
		
		if (ImGui::BeginTable(id, 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_None, 0.4f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_None, 0.6f);
		} else {
			useTableLayout_ = false;
		}
	}

	void EndTableLayout() {
		if (useTableLayout_) {
			ImGui::EndTable();
			useTableLayout_ = false;
		}
	}

	void PropertyText(const char* label, const char* fmt, ...) {
		BeginPropertyRow(label);
		
		va_list args;
		va_start(args, fmt);
		ImGui::TextV(fmt, args);
		va_end(args);
	}

	//===================================================================*/
	//		Section Filter Helpers
	//===================================================================*/
	CalyxEditor::ParamFilterSection currentFilter_ = CalyxEditor::ParamFilterSection::All;
	// セクションが表示対象か
	static bool isSectionActive_ = true; 
	static bool isSectionHeaderOpen_ = true;

	void SetSectionFilter(CalyxEditor::ParamFilterSection sectionType) {
		currentFilter_ = sectionType;
	}

	bool BeginSection(CalyxEditor::ParamFilterSection sectionType) {
		bool isAll = (currentFilter_ == CalyxEditor::ParamFilterSection::All);
		
		// フィルタリング判定
		if (!isAll) {
			if (currentFilter_ == sectionType) {
				isSectionActive_ = true;
				isSectionHeaderOpen_ = true; 
				return true;
			} else {
				isSectionActive_ = false;
				return false;
			}
		}

		// All選択中
		isSectionActive_ = true;
		isSectionHeaderOpen_ = true;
		
		return true;
	}

	void EndSection() {
		// 必要ならインデント戻しなど
		isSectionActive_ = true; 
		isSectionHeaderOpen_ = true;
	}

} // namespace GuiCmd