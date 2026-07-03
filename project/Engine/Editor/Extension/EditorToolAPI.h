#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include <cstdint>

namespace Calyx {
	struct ProjectInfo;
}

namespace CalyxEngine {
	class SceneManager;
}

namespace CalyxEditor {

	inline constexpr std::uint32_t kEditorToolApiVersion = 1;

	struct EditorToolContext {
		const Calyx::ProjectInfo* project = nullptr;
		CalyxEngine::SceneManager* sceneManager = nullptr;
	};

	class IEditorTool {
	public:
		virtual ~IEditorTool() = default;
		virtual void OnOpen() {}
		virtual void OnClose() {}
		virtual void Update() {}
		virtual void Draw() = 0;
	};

	using CreateEditorToolFn = IEditorTool* (*)(const EditorToolContext& context);
	using DestroyEditorToolFn = void (*)(IEditorTool* tool);

	// DLL boundary descriptor. Strings are copied by the engine during registration.
	// create and destroy must both point to functions in the registering module.
	struct EditorToolDescriptor {
		const char* id = nullptr;
		const char* displayName = nullptr;
		const char* menuPath = nullptr;
		const char* workspaceId = nullptr;
		const char* layoutPath = nullptr;
		CreateEditorToolFn create = nullptr;
		DestroyEditorToolFn destroy = nullptr;
	};

	class IEditorHost {
	public:
		virtual ~IEditorHost() = default;
		virtual bool RegisterTool(const EditorToolDescriptor& descriptor) = 0;
		virtual const EditorToolContext& GetContext() const = 0;
	};

	using RegisterEditorToolsFn = bool (*)(std::uint32_t apiVersion, IEditorHost* host);

} // namespace CalyxEditor

