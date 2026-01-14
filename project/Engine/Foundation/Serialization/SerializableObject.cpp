#include "SerializableObject.h"
#include "SerializableUtil.h"
#include "ParamStore.h"
#include "imgui/imgui.h"

namespace CalyxEngine {

	bool SerializableObject::SaveParams() const { return ParamStore::Save(*this); }

	bool SerializableObject::LoadParams() { return ParamStore::Load(*this); }

	void SerializableObject::SaveAndLoadButtonGui() {
		if(ImGui::Button("Load Params")) { LoadParams(); }
		ImGui::SameLine();
		if(ImGui::Button("Save Params")) { SaveParams(); }
	}

	void SerializableObject::ShowInspector() {
		VariableCategoryNode root;
		BuildCategoryTree(root, Fields());

		for (const auto& [_, node] : root.children) {
			DrawCategoryNode(node);
		}
	}

} // namespace CalyxEngine