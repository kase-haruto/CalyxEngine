#include "EditorToolRegistry.h"

#include <externals/imgui/imgui.h>

#include <algorithm>
#include <map>
#include <sstream>

namespace CalyxEditor {

	EditorToolRegistry::~EditorToolRegistry() {
		Shutdown();
	}

	bool EditorToolRegistry::RegisterTool(const EditorToolDescriptor& descriptor) {
		if(!registeringOwner_ || !descriptor.id || !descriptor.id[0] ||
		   !descriptor.displayName || !descriptor.displayName[0] ||
		   !descriptor.create || !descriptor.destroy || Find(descriptor.id)) {
			return false;
		}

		Entry entry;
		entry.id = descriptor.id;
		entry.displayName = descriptor.displayName;
		entry.menuPath = descriptor.menuPath ? descriptor.menuPath : "";
		entry.workspaceId = descriptor.workspaceId ? descriptor.workspaceId : "";
		entry.layoutPath = descriptor.layoutPath ? descriptor.layoutPath : "";
		entry.create = descriptor.create;
		entry.destroy = descriptor.destroy;
		entry.owner = registeringOwner_;
		entries_.push_back(std::move(entry));
		return true;
	}

	bool EditorToolRegistry::RegisterModule(void* owner, RegisterEditorToolsFn entryPoint) {
		if(!owner || !entryPoint || registeringOwner_) return false;
		registeringOwner_ = owner;
		const bool result = entryPoint(kEditorToolApiVersion, this);
		registeringOwner_ = nullptr;
		if(!result) UnregisterModule(owner);
		return result;
	}

	void EditorToolRegistry::UnregisterModule(void* owner) {
		for(auto& entry : entries_) {
			if(entry.owner == owner) Destroy(entry);
		}
		std::erase_if(entries_, [owner](const Entry& entry) { return entry.owner == owner; });
	}

	void EditorToolRegistry::Shutdown() {
		for(auto& entry : entries_) Destroy(entry);
		entries_.clear();
	}

	void EditorToolRegistry::DrawToolsMenu() {
		struct MenuNode {
			std::map<std::string, MenuNode> children;
			Entry* command = nullptr;
		};

		MenuNode root;
		for(auto& entry : entries_) {
			if(entry.menuPath.empty()) {
				root.children[entry.displayName].command = &entry;
				continue;
			}

			MenuNode* node = &root;
			std::istringstream path(entry.menuPath);
			std::string segment;
			while(std::getline(path, segment, '/')) {
				if(!segment.empty()) node = &node->children[segment];
			}
			if(node == &root || node->command) {
				node = &node->children[entry.displayName];
			}
			node->command = &entry;
		}

		auto drawNode = [this](auto&& self, MenuNode& node) -> void {
			for(auto& [label, child] : node.children) {
				if(child.command && child.children.empty()) {
					if(ImGui::MenuItem(label.c_str())) Open(child.command->id);
				} else if(ImGui::BeginMenu(label.c_str())) {
					if(child.command && ImGui::MenuItem(child.command->displayName.c_str())) {
						Open(child.command->id);
					}
					self(self, child);
					ImGui::EndMenu();
				}
			}
		};
		drawNode(drawNode, root);
	}

	void EditorToolRegistry::Update() {
		for(auto& entry : entries_) {
			if(entry.instance) entry.instance->Update();
		}
	}

	void EditorToolRegistry::Draw() {
		for(auto& entry : entries_) {
			if(!entry.instance) continue;
			entry.instance->Draw();
			if(!entry.instance->IsOpen()) Destroy(entry);
		}
	}

	bool EditorToolRegistry::Open(std::string_view id) {
		auto* entry = Find(id);
		if(!entry) return false;
		if(!entry->instance) {
			entry->instance = entry->create(context_);
			if(!entry->instance) return false;
		}
		entry->instance->OnOpen();
		if(workspaceRequest_ && !entry->layoutPath.empty()) workspaceRequest_(entry->layoutPath);
		return true;
	}

	bool EditorToolRegistry::Close(std::string_view id) {
		auto* entry = Find(id);
		if(!entry || !entry->instance) return false;
		Destroy(*entry);
		return true;
	}

	EditorToolRegistry::Entry* EditorToolRegistry::Find(std::string_view id) {
		auto it = std::find_if(entries_.begin(), entries_.end(), [id](const Entry& entry) { return entry.id == id; });
		return it == entries_.end() ? nullptr : &*it;
	}

	void EditorToolRegistry::Destroy(Entry& entry) {
		if(!entry.instance) return;
		entry.instance->OnClose();
		entry.destroy(entry.instance);
		entry.instance = nullptr;
	}

} // namespace CalyxEditor
