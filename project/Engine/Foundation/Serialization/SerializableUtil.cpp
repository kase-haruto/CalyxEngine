#include "SerializableUtil.h"

#include "SerializableField.h"
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>


std::vector<std::string> CalyxEngine::SplitCategory(const std::string& s) {
	std::vector<std::string> result;
	size_t                   start = 0;
	while(true) {
		size_t pos = s.find('|',start);
		if(pos == std::string::npos) {
			result.push_back(s.substr(start));
			break;
		}
		result.push_back(s.substr(start,pos - start));
		start = pos + 1;
	}
	return result;
}

void CalyxEngine::BuildCategoryTree(VariableCategoryNode& root,const std::vector<SerializableField>& fields) {
	for(const auto& f : fields) {
		if(f.hidden) continue;

		const std::string& cat =
			f.category.empty() ? "Default" : f.category;

		auto                  path = SplitCategory(cat);
		VariableCategoryNode* node = &root;

		for(const auto& p : path) {
			node       = &node->children[p];
			node->name = p;
		}
		node->fields.push_back(&f);
	}
}

bool CalyxEngine::DrawField(const SerializableField& f) {
	bool changed = false;

	ImGui::PushID(&f);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(f.key.c_str());

	if(!f.tooltip.empty() && ImGui::IsItemHovered()) { ImGui::SetTooltip("%s",f.tooltip.c_str()); }

	ImGui::SameLine(220.0f);

	std::visit([&]<typename P>(P* p) {
		using RawT = std::remove_pointer_t<P>;
		using T    = std::remove_const_t<RawT>;

		// =========================
		// 表示のみ（読み取り専用）
		// =========================
		auto drawText = [&]() {
			if constexpr(std::is_same_v<T,int32_t>)
				ImGui::Text("%d",*p);
			else if constexpr(std::is_same_v<T,size_t>)
				ImGui::Text("%zu",*p);
			else if constexpr(std::is_same_v<T,float>)
				ImGui::Text("%.3f",*p);
			else if constexpr(std::is_same_v<T,bool>)
				ImGui::Text(*p ? "true" : "false");
			else if constexpr(std::is_same_v<T,CalyxMath::Vector2>)
				ImGui::Text("(%.2f, %.2f)",p->x,p->y);
			else if constexpr(std::is_same_v<T,CalyxMath::Vector3>)
				ImGui::Text("(%.2f, %.2f, %.2f)",p->x,p->y,p->z);
			else if constexpr(std::is_same_v<T,CalyxMath::Vector4>)
				ImGui::Text("(%.2f, %.2f, %.2f, %.2f)",p->x,p->y,p->z,p->w);
		};

		if constexpr(std::is_const_v<RawT>) { drawText(); } else {
			if(f.readOnly) { drawText(); } else {
				// =========================
				// 編集可
				// =========================
				if constexpr(std::is_same_v<T,int32_t>)
					changed |= GuiCmd::DragInt("##v",*p,1.0f);
				else if constexpr(std::is_same_v<T,size_t>) {
					int temp = static_cast<int>(*p);
					if(GuiCmd::DragInt("##v",temp,1.0f,f.min,f.max)) {
						*p      = static_cast<size_t>(std::max(0,temp));
						changed = true;
					}
				} else if constexpr(std::is_same_v<T,float>)
					changed |= (f.hasRange
									? GuiCmd::SliderFloat("##v",*p,f.min,f.max)
									: GuiCmd::DragFloat("##v",*p,f.speed));
				else if constexpr(std::is_same_v<T,bool>)
					changed |= GuiCmd::CheckBox("##v",*p);
				else if constexpr(std::is_same_v<T,CalyxMath::Vector2>)
					changed |= GuiCmd::DragFloat2("##v",*p,f.speed);
				else if constexpr(std::is_same_v<T,CalyxMath::Vector3>)
					changed |= GuiCmd::DragFloat3("##v",*p,f.speed);
				else if constexpr(std::is_same_v<T,CalyxMath::Vector4>)
					changed |= GuiCmd::DragFloat4("##v",*p,f.speed);
			}
		}
	},f.ptr);

	ImGui::PopID();

	return changed;
}

bool CalyxEngine::DrawCategoryNode(const VariableCategoryNode& node) {

	bool anyChanged = false;

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_SpanFullWidth |
		ImGuiTreeNodeFlags_FramePadding;

	bool open = ImGui::TreeNodeEx(node.name.c_str(),flags);
	if(!open) return false;

	for(const auto& [_, child] : node.children) { anyChanged |= DrawCategoryNode(child); }

	for(const auto* f : node.fields) { anyChanged |= DrawField(*f); }

	ImGui::TreePop();

	return anyChanged;
}