#include "SerializableUtil.h"

#include "SerializableField.h"
#include "SerializableObject.h"
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

std::vector<std::string> CalyxEngine::SplitCategory(const std::string& s) {
	std::vector<std::string> result;
	size_t					 start = 0;
	while(true) {
		size_t pos = s.find('|', start);
		if(pos == std::string::npos) {
			result.push_back(s.substr(start));
			break;
		}
		result.push_back(s.substr(start, pos - start));
		start = pos + 1;
	}
	return result;
}

void CalyxEngine::BuildCategoryTree(VariableCategoryNode& root, const std::vector<SerializableField>& fields) {
	for(const auto& f : fields) {
		if(f.hidden) continue;

		const std::string& cat =
			f.category.empty() ? "Default" : f.category;

		auto				  path = SplitCategory(cat);
		VariableCategoryNode* node = &root;

		for(const auto& p : path) {
			node	   = &node->children[p];
			node->name = p;
		}
		node->fields.push_back(&f);
	}
}

bool CalyxEngine::DrawField(const SerializableField& f) {
	bool changed = false;

	ImGui::PushID(&f);

	std::visit([&]<typename P>(P* p) {
		using RawT = std::remove_pointer_t<P>;
		using T	   = std::remove_const_t<RawT>;

		const char* label = f.key.c_str();

		// =========================
		// 表示のみ（読み取り専用）
		// =========================
		auto drawReadOnly = [&]() {
			if constexpr(std::is_same_v<T, int32_t>)
				GuiCmd::PropertyText(label, "%d", *p);
			else if constexpr(std::is_same_v<T, size_t>)
				GuiCmd::PropertyText(label, "%zu", *p);
			else if constexpr(std::is_same_v<T, float>)
				GuiCmd::PropertyText(label, "%.3f", *p);
			else if constexpr(std::is_same_v<T, bool>)
				GuiCmd::PropertyText(label, *p ? "true" : "false");
			else if constexpr(std::is_same_v<T, CalyxEngine::Vector2>)
				GuiCmd::PropertyText(label, "(%.2f, %.2f)", p->x, p->y);
			else if constexpr(std::is_same_v<T, CalyxEngine::Vector3>)
				GuiCmd::PropertyText(label, "(%.2f, %.2f, %.2f)", p->x, p->y, p->z);
			else if constexpr(std::is_same_v<T, CalyxEngine::Vector4>)
				GuiCmd::PropertyText(label, "(%.2f, %.2f, %.2f, %.2f)", p->x, p->y, p->z, p->w);
			else if constexpr(std::is_same_v<T, CalyxEngine::Quaternion>)
				GuiCmd::PropertyText(label, "(%.2f, %.2f, %.2f, %.2f)", p->x, p->y, p->z, p->w);
		};

		if constexpr(std::is_const_v<RawT>) {
			drawReadOnly();
		} else {
			if(f.readOnly) {
				drawReadOnly();
			} else {
				// =========================
				// 編集可
				// =========================
				if constexpr(std::is_same_v<T, int32_t>)
					changed |= GuiCmd::DragInt(label, *p, 1.0f);
				else if constexpr(std::is_same_v<T, size_t>) {
					int temp = static_cast<int>(*p);
					if(GuiCmd::DragInt(label, temp, 1.0f, f.min, f.max)) {
						*p		= static_cast<size_t>(std::max(0, temp));
						changed = true;
					}
				} else if constexpr(std::is_same_v<T, float>)
					changed |= (f.hasRange
									? GuiCmd::SliderFloat(label, *p, f.min, f.max)
									: GuiCmd::DragFloat(label, *p, f.speed));
				else if constexpr(std::is_same_v<T, bool>)
					changed |= GuiCmd::CheckBox(label, *p);
				else if constexpr(std::is_same_v<T, CalyxEngine::Vector2>)
					changed |= GuiCmd::DragFloat2(label, *p, f.speed);
				else if constexpr(std::is_same_v<T, CalyxEngine::Vector3>)
					changed |= GuiCmd::DragFloat3(label, *p, f.speed);
				else if constexpr(std::is_same_v<T, CalyxEngine::Vector4>)
					changed |= GuiCmd::DragFloat4(label, *p, f.speed);
				else if constexpr(std::is_same_v<T, CalyxEngine::Quaternion>)
					changed |= GuiCmd::DragFloat4(label, reinterpret_cast<CalyxEngine::Vector4&>(*p), f.speed);
			}
		}

		if(!f.tooltip.empty() && ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", f.tooltip.c_str());
		}
	},
			   f.ptr);

	ImGui::PopID();

	return changed;
}

bool CalyxEngine::DrawCategoryNode(const VariableCategoryNode& node, SerializableObject* owner) {

	bool anyChanged = false;

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_SpanFullWidth |
		ImGuiTreeNodeFlags_FramePadding;

	bool open = ImGui::TreeNodeEx(node.name.c_str(), flags);
	if(!open) return false;

	// Categorized Save/Load buttons for root levels
	if(owner) {
		owner->SaveAndLoadButtonGui();
		ImGui::Separator();
	}

	// テーブルレイアウト開始
	GuiCmd::BeginTableLayout();

	for(const auto& [_, child] : node.children) {
		GuiCmd::EndTableLayout();
		anyChanged |= DrawCategoryNode(child, nullptr);
		GuiCmd::BeginTableLayout();
	}

	for(const auto* f : node.fields) {
		anyChanged |= DrawField(*f);
	}

	GuiCmd::EndTableLayout();

	ImGui::TreePop();

	return anyChanged;
}
