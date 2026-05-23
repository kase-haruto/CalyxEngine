#pragma once

#include <Engine\Editor\NodeEditor\NodeGraph.h>
#include <externals\imgui\imgui_node_editor.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace CalyxEngine {
	class NodeEditorCanvas {
	public:
		using DrawNodeBody = std::function<bool(Node&)>;
		enum class ContextMenuType {
			Background,
			Node
		};
		struct ContextMenu {
			ContextMenuType type = ContextMenuType::Background;
			Vector2 canvasPosition{};
			int32_t nodeId = 0;
		};
		using DrawContextMenu = std::function<bool(const ContextMenu&)>;
		using GraphMutationCommand = std::function<void(const char* name, const NodeGraph& before, const NodeGraph& after)>;

		explicit NodeEditorCanvas(std::string id);
		~NodeEditorCanvas();

		NodeEditorCanvas(const NodeEditorCanvas&) = delete;
		NodeEditorCanvas& operator=(const NodeEditorCanvas&) = delete;

		bool Draw(NodeGraph& graph, const DrawNodeBody& drawBody = {}, const DrawContextMenu& drawContextMenu = {}, const GraphMutationCommand& graphMutationCommand = {});
		bool ConsumeBackgroundContextRequest(Vector2& outCanvasPos);
		bool ConsumeNodeContextRequest(int32_t& outNodeId);
		Vector2 GetLastViewCenter() const { return lastViewCenter_; }
		int32_t GetSelectedNodeId() const { return selectedNodeId_; }

	private:
		bool CanCreateLink(const NodeGraph& graph, int32_t a, int32_t b, int32_t& from, int32_t& to) const;
		void DrawNodePins(const Node& node);
		void DrawPin(const NodePin& pin, float rowWidth);
		void DrawNodeHeader(const Node& node);
		ImVec4 GetNodeHeaderColor(const Node& node) const;
		ImVec4 GetPinColor(NodeValueType type) const;
		float GetLinkThickness(NodeValueType type) const;

	private:
		std::string id_;
		ax::NodeEditor::EditorContext* context_ = nullptr;
		const NodeGraph* lastGraph_ = nullptr;
		std::unordered_set<int32_t> positionedNodes_;
		bool backgroundContextRequested_ = false;
		bool nodeContextRequested_ = false;
		Vector2 contextCanvasPos_{};
		int32_t contextNodeId_ = 0;
		ContextMenu activeContextMenu_{};
		bool hasActiveContextMenu_ = false;
		Vector2 lastViewCenter_{};
		int32_t selectedNodeId_ = 0;
	};
}
