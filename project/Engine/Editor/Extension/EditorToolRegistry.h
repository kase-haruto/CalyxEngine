#pragma once

#include <Engine/Editor/Extension/EditorToolAPI.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace CalyxEditor {

	class CALYX_API EditorToolRegistry final : public IEditorHost {
	public:
		using WorkspaceRequest = std::function<void(const std::string& layoutPath)>;

		~EditorToolRegistry() override;

		bool RegisterTool(const EditorToolDescriptor& descriptor) override;
		const EditorToolContext& GetContext() const override { return context_; }

		void SetContext(EditorToolContext context) { context_ = context; }
		void SetWorkspaceRequest(WorkspaceRequest request) { workspaceRequest_ = std::move(request); }

		bool RegisterModule(void* owner, RegisterEditorToolsFn entryPoint);
		void UnregisterModule(void* owner);
		void Shutdown();

		void DrawToolsMenu();
		void Update();
		void Draw();
		bool Open(std::string_view id);
		bool Close(std::string_view id);

	private:
		struct Entry {
			std::string id;
			std::string displayName;
			std::string menuPath;
			std::string workspaceId;
			std::string layoutPath;
			CreateEditorToolFn create = nullptr;
			DestroyEditorToolFn destroy = nullptr;
			IEditorTool* instance = nullptr;
			void* owner = nullptr;
		};

		Entry* Find(std::string_view id);
		void Destroy(Entry& entry);

		std::vector<Entry> entries_;
		EditorToolContext context_;
		WorkspaceRequest workspaceRequest_;
		void* registeringOwner_ = nullptr;
	};

} // namespace CalyxEditor

