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

// exterunal
#include <externals/imgui/imgui_internal.h>

/* ==========================================================================================================
/*			DragInt
/* ======================================================================================================== */
#pragma region DragInt

bool GuiCmd::DragInt(const char* label, int& value, float speed, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<int> computer;
	int temp = value;
	bool changed = ImGui::DragInt(label, &temp, speed, static_cast< int >(min), static_cast< int >(max));
	value = temp;

	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const int& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd){
			CommandManager::GetInstance()->Execute(std::move(cmd));
		}
	}
	return changed;
}

#pragma endregion

/* ==========================================================================================================
/*			DragFloat
/* ======================================================================================================== */
#pragma region DragFloat

bool GuiCmd::DragFloat(const char* label, float& value, float speed, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<float> computer;
	float temp = value;
	bool changed = ImGui::DragFloat(label, &temp, speed, min, max);
	value = temp;

	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const float& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd){
			CommandManager::GetInstance()->Execute(std::move(cmd));		
		}
	}
	return changed;
}

bool GuiCmd::DragFloat2(const char* label, CxMath::Vector2& value, float speed, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector2> computer;
	CxMath::Vector2 temp = value;
	bool changed = ImGui::DragFloat2(label, &temp.x, speed, min, max);
	value = temp;
	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector2& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}

bool GuiCmd::DragFloat3(const char* label, CxMath::Vector3& value, float speed, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector3> computer;
	CxMath::Vector3 temp = value;
	bool changed = ImGui::DragFloat3(label, &temp.x, speed, min, max);
	value = temp;

	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);

	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector3& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}

bool GuiCmd::DragFloat4(const char* label, CxMath::Vector4& value, float speed, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector4> computer;
	CxMath::Vector4 temp = value;
	bool changed = ImGui::DragFloat4(label, &temp.x, speed, min, max);
	value = temp;
	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector4& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}


bool GuiCmd::ColoredDragFloat3(const char* label,
							   CxMath::Vector3& value,
							   float speed,
							   float min,
							   float max,
							   const char* format,
							   const char* suffix){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector3> computer;
	CxMath::Vector3 temp = value;
	bool changed = false;

	const char* axisLabels[3] = {"X", "Y", "Z"};
	const ImVec4 axisColors[3] = {
		ImVec4(1.0f, 0.4f, 0.4f, 1.0f), // Red
		ImVec4(0.5f, 1.0f, 0.5f, 1.0f), // Green
		ImVec4(0.5f, 0.7f, 1.0f, 1.0f)  // Blue
	};

	ImGui::Text("%s", label);
	ImGui::PushID(label);

	for (int i = 0; i < 3; ++i){
		ImGui::PushID(i);
		ImGui::PushStyleColor(ImGuiCol_Text, axisColors[i]);
		ImGui::Text("%s", axisLabels[i]);
		ImGui::SameLine();
		ImGui::PopStyleColor();

		std::string fmt = format ? format : "%.3f";
		if (suffix && suffix[0] != '\0'){
			fmt += std::string(suffix);
		}

		ImGui::PushItemWidth(60.0f);
		changed |= ImGui::DragFloat("##v", &(&temp.x)[i], speed, min, max, fmt.c_str());
		ImGui::PopItemWidth();

		if (i < 2) ImGui::SameLine();
		ImGui::PopID();
	}

	ImGui::PopID();

	if (changed)
		value = temp;

	if (ImGui::IsItemActivated()) computer.Begin(value);

	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector3& v){ value = v; }, labelStr);
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}

	return changed;
}


#pragma endregion

/* ==========================================================================================================
/*			SliderFloat
/* ======================================================================================================== */
#pragma region SliderFloat
bool GuiCmd::SliderFloat(const char* label, float& value, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<float> computer;
	float temp = value;
	bool changed = ImGui::SliderFloat(label, &temp, min, max);
	value = temp;
	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const float& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}

bool GuiCmd::SliderFloat2(const char* label, CxMath::Vector2& value, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector2> computer;
	CxMath::Vector2 temp = value;
	bool changed = ImGui::SliderFloat2(label, &temp.x, min, max);
	value = temp;
	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector2& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}

bool GuiCmd::SliderFloat3(const char* label, CxMath::Vector3& value, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector3> computer;
	CxMath::Vector3 temp = value;
	bool changed = ImGui::SliderFloat3(label, &temp.x, min, max);
	value = temp;
	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector3& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}

bool GuiCmd::SliderFloat4(const char* label, CxMath::Vector4& value, float min, float max){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector4> computer;
	CxMath::Vector4 temp = value;
	bool changed = ImGui::SliderFloat3(label, &temp.x, min, max);
	value = temp;
	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector4& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}
#pragma endregion

/* ==========================================================================================================
/*			ColorEdit
/* ======================================================================================================== */
#pragma region ColorEdit
bool GuiCmd::ColorEdit4(const char* label, CxMath::Vector4& value, ImGuiColorEditFlags flags){
	static GuiCmdInternal::GuiCmdSetValueComputer<CxMath::Vector4> computer;

	CxMath::Vector4 temp = value;
	bool changed = ImGui::ColorEdit4(label, &temp.x, flags);

	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()){
		computer.Begin(value);
	}

	value = temp;
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const CxMath::Vector4& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}

#pragma endregion

/* ==========================================================================================================
/*			checkBox
/* ======================================================================================================== */
#pragma region CheckBox
bool GuiCmd::CheckBox(const char* label, bool& value){
	static GuiCmdInternal::GuiCmdSetValueComputer<bool> computer;
	bool temp = value;
	bool changed = ImGui::Checkbox(label, &temp);
	value = temp;

	// マウスが押された 検知開始
	if (ImGui::IsItemActivated()) computer.Begin(value);
	// マウスが離れた
	if (ImGui::IsItemDeactivatedAfterEdit()){
		std::string labelStr(label);
		auto cmd = computer.End(value, [&value] (const bool& v){ value = v; }, labelStr);
		//マウスが押された位置から動いていたらコマンドを発行する
		if (cmd) CommandManager::GetInstance()->Execute(std::move(cmd));
	}
	return changed;
}
#pragma endregion