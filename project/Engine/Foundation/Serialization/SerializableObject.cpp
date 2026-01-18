#include "SerializableObject.h"
#include "SerializableUtil.h"
#include "ParamStore.h"
#include "imgui/imgui.h"

namespace CalyxEngine {

	bool SerializableObject::SaveParams() const { return ParamStore::Save(*this); }

	bool SerializableObject::LoadParams() { return ParamStore::Load(*this); }

	void SerializableObject::SaveAndLoadButtonGui() {
		auto path = GetParamPath();
		std::string loadLabel = "Load " + path.name;
		std::string saveLabel = "Save " + path.name;

		if(ImGui::Button(loadLabel.c_str())) { LoadParams(); }
		ImGui::SameLine();
		if(ImGui::Button(saveLabel.c_str())) { SaveParams(); }
	}

	bool SerializableObject::ShowGui() {
		VariableCategoryNode root;
		BuildCategoryTree(root, Fields());

		bool changed = false;
		for (const auto& [_, node] : root.children) {
			// Integrate Save/Load buttons into each root category tab
			changed |= DrawCategoryNode(node, this);
		}

		return changed;
	}

} // namespace CalyxEngine