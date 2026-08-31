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
		// Module登録中のみToolを受理し、所有DLLとcreate/destroy関数の対応を保証する。
		if(!registeringOwner_ || !descriptor.id || !descriptor.id[0] ||
		   !descriptor.displayName || !descriptor.displayName[0] ||
		   !descriptor.create || !descriptor.destroy || Find(descriptor.id)) {
			return false;
		}

		// DLL側文字列のLifetimeに依存しないよう、Descriptor文字列をRegistryへCopyする。
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
		// Entry Point実行中だけOwnerをContextとして設定し、登録ToolへModule Handleを関連付ける。
		registeringOwner_ = owner;
		const bool result = entryPoint(kEditorToolApiVersion, this);
		registeringOwner_ = nullptr;
		// Module側登録が部分失敗した場合は、そのModuleが追加したToolをすべてRollbackする。
		if(!result) UnregisterModule(owner);
		return result;
	}

	void EditorToolRegistry::UnregisterModule(void* owner) {
		// DLLをUnloadする前にInstanceをModule提供のdestroy関数で破棄する。
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

		// slash区切りのmenuPathを一時Treeへ変換し、任意階層のImGui Menuを構築する。
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

		// Menu Treeを再帰描画し、Leaf選択時だけTool Instanceを開く。
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
			// Tool自身がWindowを閉じた場合は同じFrameで破棄し、次Frameへ無効Instanceを残さない。
			entry.instance->Draw();
			if(!entry.instance->IsOpen()) Destroy(entry);
		}
	}

	bool EditorToolRegistry::Open(std::string_view id) {
		auto* entry = Find(id);
		if(!entry) return false;
		// Instanceは初回Openまで遅延生成し、未使用Plugin ToolのResource確保を避ける。
		if(!entry->instance) {
			entry->instance = entry->create(context_);
			if(!entry->instance) return false;
		}
		entry->instance->OnOpen();
		// Tool指定Layoutがある場合だけEditor Workspaceへ切替要求を通知する。
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
		// DLL境界を跨ぐObjectはホスト側deleteを使わず、生成元Moduleのdestroy関数へ返す。
		entry.instance->OnClose();
		entry.destroy(entry.instance);
		entry.instance = nullptr;
	}

} // namespace CalyxEditor
