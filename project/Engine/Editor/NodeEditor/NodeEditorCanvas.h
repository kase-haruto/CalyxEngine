#pragma once

#include <Engine\Editor\NodeEditor\NodeGraph.h>
#include <externals\imgui\imgui_node_editor.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * NodeEditorCanvas
	 * - 汎用NodeGraphをImGui Node Editor上で編集するEditor専用クラス
	 * - Node、Pin、Linkの描画と編集イベントの収集を担当する
	 * - 用途固有の評価処理やGraphリソースの所有は担当しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief NodeEditorCanvasの機能を提供するクラスです。
	 */
	class NodeEditorCanvas {
public:
		/*-----------------------------------------------------------------------------------------
		 * Policy
		 * - NodeEditorCanvasへ用途固有の接続規則と外観を注入するデータ構造
		 * - Callbackの呼び出し先とそのライフタイムは所有しない
		 *---------------------------------------------------------------------------------------*/
		/**
		 * @brief Policyに関するデータを保持する構造体です。
		 */
		struct Policy {
			std::function<bool(const NodePin&, const NodePin&)> canConnect;
			std::function<ImVec4(const Node&)> nodeHeaderColor;
			std::function<ImVec4(std::string_view)> pinColor;
		};

		using DrawNodeBody = std::function<bool(Node&)>;
		enum class ContextMenuType {
			Background,
			Node
		};
		/*-----------------------------------------------------------------------------------------
		 * ContextMenu
		 * - Node Editorから要求されたContext Menuの情報を保持するデータ構造
		 * - Menu種別、Canvas座標、対象Node IDを呼び出し側へ通知する
		 *---------------------------------------------------------------------------------------*/
		/**
		 * @brief ContextMenuに関するデータを保持する構造体です。
		 */
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
		void SetPolicy(Policy policy) { policy_ = std::move(policy); }

	private:
		bool CanCreateLink(const NodeGraph& graph, int32_t a, int32_t b, int32_t& from, int32_t& to) const;
		void DrawNodePins(const Node& node);
		void DrawPin(const NodePin& pin, float rowWidth);
		void DrawNodeHeader(const Node& node);
		ImVec4 GetNodeHeaderColor(const Node& node) const;
		ImVec4 GetPinColor(std::string_view pinType) const;
		float GetLinkThickness(std::string_view pinType) const;

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
		Policy policy_{};
	};
}
