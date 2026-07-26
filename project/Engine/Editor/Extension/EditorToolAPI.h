#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>

#include <cstdint>

namespace Calyx {
	struct ProjectInfo;
}

namespace CalyxEngine {
	class SceneManager;
}

class BaseCamera;
class SceneObject;

namespace CalyxEditor {

	inline constexpr std::uint32_t kEditorToolApiVersion = 2;

	/**
	 * @brief EditorToolContextに関するデータを保持する構造体です。
	 */
	struct EditorToolContext {
		const Calyx::ProjectInfo* project = nullptr;
		CalyxEngine::SceneManager* sceneManager = nullptr;
		void* editorUserData = nullptr;
		SceneObject* (*getPrimarySelection)(void* userData) = nullptr;
		BaseCamera* (*getMainCamera)(void* userData) = nullptr;
		bool (*isPlaying)(void* userData) = nullptr;
		void (*requestSaveScene)(void* userData) = nullptr;

		SceneObject* GetPrimarySelection() const {
			return getPrimarySelection ? getPrimarySelection(editorUserData) : nullptr;
		}
		BaseCamera* GetMainCamera() const {
			return getMainCamera ? getMainCamera(editorUserData) : nullptr;
		}
		bool IsPlaying() const { return isPlaying && isPlaying(editorUserData); }
		void RequestSaveScene() const {
			if(requestSaveScene) requestSaveScene(editorUserData);
		}
	};

	/**
	 * @brief IEditorToolの機能を提供するクラスです。
	 */
	class IEditorTool {
	public:
		virtual ~IEditorTool() = default;
		virtual void OnOpen() {}
		virtual void OnClose() {}
		virtual void Update() {}
		virtual void Draw() = 0;
		// Return false after the tool window is closed. The host then calls
		// OnClose and destroys the instance in the module that created it.
		virtual bool IsOpen() const { return true; }
	};

	using CreateEditorToolFn = IEditorTool* (*)(const EditorToolContext& context);
	using DestroyEditorToolFn = void (*)(IEditorTool* tool);

	// DLL boundary descriptor. Strings are copied by the engine during registration.
	// create and destroy must both point to functions in the registering module.
	/**
	 * @brief EditorToolDescriptorに関するデータを保持する構造体です。
	 */
	struct EditorToolDescriptor {
		const char* id = nullptr;
		const char* displayName = nullptr;
		const char* menuPath = nullptr;
		const char* workspaceId = nullptr;
		const char* layoutPath = nullptr;
		CreateEditorToolFn create = nullptr;
		DestroyEditorToolFn destroy = nullptr;
	};

	/**
	 * @brief IEditorHostの機能を提供するクラスです。
	 */
	class IEditorHost {
	public:
		virtual ~IEditorHost() = default;
		virtual bool RegisterTool(const EditorToolDescriptor& descriptor) = 0;
		virtual const EditorToolContext& GetContext() const = 0;
	};

	using RegisterEditorToolsFn = bool (*)(std::uint32_t apiVersion, IEditorHost* host);

} // namespace CalyxEditor

