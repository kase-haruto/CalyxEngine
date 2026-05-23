#include "NodeEditorCanvas.h"

#include <externals\imgui\imgui.h>

#include <algorithm>

namespace ed = ax::NodeEditor;

namespace CalyxEngine {
	namespace {
		struct BlueprintNodeStyle {
			float nodeWidth = 220.0f;
			float headerHeight = 30.0f;
			float pinRadius = 6.0f;
			float pinHitSize = 18.0f;
			float rowHeight = 24.0f;
			float bodyPaddingX = 12.0f;
			float bodyPaddingY = 8.0f;
			float pinLabelSpacing = 7.0f;
			float pinColumnGap = 12.0f;
			float linkThickness = 2.6f;
			float validPreviewThickness = 3.2f;
			float invalidPreviewThickness = 2.2f;
			ImVec4 bg = ImVec4(0.060f, 0.065f, 0.072f, 1.0f);
			ImVec4 grid = ImVec4(0.55f, 0.58f, 0.62f, 0.10f);
			ImVec4 nodeBg = ImVec4(0.105f, 0.112f, 0.122f, 0.98f);
			ImVec4 nodeBorder = ImVec4(0.30f, 0.33f, 0.36f, 0.95f);
			ImVec4 hoveredBorder = ImVec4(0.42f, 0.64f, 0.86f, 1.0f);
			ImVec4 selectedBorder = ImVec4(1.00f, 0.62f, 0.18f, 1.0f);
			ImVec4 text = ImVec4(0.88f, 0.91f, 0.94f, 1.0f);
			ImVec4 textMuted = ImVec4(0.62f, 0.67f, 0.72f, 1.0f);
			ImVec4 pinOutline = ImVec4(0.030f, 0.035f, 0.042f, 1.0f);
			ImVec4 pinHighlight = ImVec4(1.00f, 1.00f, 1.00f, 0.26f);
			ImVec4 pinShadow = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);
			ImVec4 reject = ImVec4(0.95f, 0.20f, 0.16f, 1.0f);
		};

		const BlueprintNodeStyle kStyle{};

		ImU32 ToU32(const ImVec4& color) {
			return ImGui::ColorConvertFloat4ToU32(color);
		}

		void DrawPinShape(ImDrawList* drawList, const ImVec2& center, NodeValueType type, const ImVec4& fill, const ImVec4& outline) {
			const float r = kStyle.pinRadius;
			drawList->AddCircleFilled(ImVec2(center.x + 1.0f, center.y + 1.0f), r + 2.0f, ToU32(kStyle.pinShadow), 18);
			switch(type) {
			case NodeValueType::Color:
				drawList->AddCircleFilled(center, r, ToU32(fill), 20);
				drawList->AddCircle(center, r + 1.5f, ToU32(outline), 20, 1.8f);
				drawList->AddCircle(ImVec2(center.x - 1.5f, center.y - 1.5f), r * 0.45f, ToU32(kStyle.pinHighlight), 12, 1.0f);
				break;
			case NodeValueType::Bool: {
				const ImVec2 pts[] = {
					ImVec2(center.x, center.y - r - 1.0f),
					ImVec2(center.x + r + 1.0f, center.y + r),
					ImVec2(center.x - r - 1.0f, center.y + r)};
				drawList->AddTriangleFilled(pts[0], pts[1], pts[2], ToU32(fill));
				drawList->AddTriangle(pts[0], pts[1], pts[2], ToU32(outline), 1.2f);
				break;
			}
			case NodeValueType::Int: {
				const ImVec2 pts[] = {
					ImVec2(center.x, center.y - r - 1.0f),
					ImVec2(center.x + r + 1.0f, center.y),
					ImVec2(center.x, center.y + r + 1.0f),
					ImVec2(center.x - r - 1.0f, center.y)};
				drawList->AddConvexPolyFilled(pts, 4, ToU32(fill));
				drawList->AddPolyline(pts, 4, ToU32(outline), ImDrawFlags_Closed, 1.2f);
				break;
			}
			case NodeValueType::Texture2D: {
				const ImVec2 min(center.x - r - 1.0f, center.y - r - 1.0f);
				const ImVec2 max(center.x + r + 1.0f, center.y + r + 1.0f);
				drawList->AddRectFilled(min, max, ToU32(fill), 2.0f);
				drawList->AddRect(min, max, ToU32(outline), 2.0f, 0, 1.4f);
				break;
			}
			default:
				drawList->AddCircleFilled(center, r, ToU32(fill), 18);
				drawList->AddCircle(center, r + 1.5f, ToU32(outline), 18, 1.8f);
				drawList->AddCircle(ImVec2(center.x - 1.5f, center.y - 1.5f), r * 0.42f, ToU32(kStyle.pinHighlight), 12, 1.0f);
				break;
			}
		}
	}

	NodeEditorCanvas::NodeEditorCanvas(std::string id) : id_(std::move(id)) {
		ed::Config config;
		config.SettingsFile = nullptr;
		config.DragButtonIndex = 0;
		config.SelectButtonIndex = 0;
		config.NavigateButtonIndex = 2;
		config.ContextMenuButtonIndex = 1;
		context_ = ed::CreateEditor(&config);
	}

	NodeEditorCanvas::~NodeEditorCanvas() {
		if(context_) ed::DestroyEditor(context_);
	}

	void NodeEditorCanvas::DrawPin(const NodePin& pin, float rowWidth) {
		const auto kind = pin.kind == NodePinKind::Input ? ed::PinKind::Input : ed::PinKind::Output;
		const ImVec4 pinColor = GetPinColor(pin.valueType);
		ed::BeginPin(ed::PinId(pin.id), kind);
		ImGui::PushID(pin.id);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		if(pin.kind == NodePinKind::Input) {
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 center(cursor.x + kStyle.pinHitSize * 0.5f, cursor.y + kStyle.rowHeight * 0.5f);
			ImGui::InvisibleButton("pin", ImVec2(kStyle.pinHitSize, kStyle.rowHeight));
			DrawPinShape(drawList, center, pin.valueType, pinColor, kStyle.pinOutline);
			ImGui::SameLine(0.0f, kStyle.pinLabelSpacing);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
			ImGui::TextColored(kStyle.textMuted, "%s", pin.name.c_str());
		} else {
			const float labelWidth = ImGui::CalcTextSize(pin.name.c_str()).x;
			ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), rowWidth - labelWidth - kStyle.pinHitSize));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
			ImGui::TextColored(kStyle.textMuted, "%s", pin.name.c_str());
			ImGui::SameLine(0.0f, kStyle.pinLabelSpacing);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3.0f);
			ImVec2 cursor = ImGui::GetCursorScreenPos();
			ImVec2 center(cursor.x + kStyle.pinHitSize * 0.5f, cursor.y + kStyle.rowHeight * 0.5f);
			ImGui::InvisibleButton("pin", ImVec2(kStyle.pinHitSize, kStyle.rowHeight));
			DrawPinShape(drawList, center, pin.valueType, pinColor, kStyle.pinOutline);
		}
		ImGui::PopID();
		ed::EndPin();
	}

	void NodeEditorCanvas::DrawNodePins(const Node& node) {
		const size_t rowCount = std::max(node.inputs.size(), node.outputs.size());
		const float contentWidth = kStyle.nodeWidth - kStyle.bodyPaddingX * 2.0f;
		for(size_t i = 0; i < rowCount; ++i) {
			ImGui::PushID(static_cast<int32_t>(i));
			const float rowStartY = ImGui::GetCursorPosY();
			if(i < node.inputs.size()) {
				DrawPin(node.inputs[i], contentWidth);
			} else {
				ImGui::Dummy(ImVec2(1.0f, kStyle.rowHeight));
			}

			if(i < node.outputs.size()) {
				ImGui::SameLine(0.0f, kStyle.pinColumnGap);
				ImGui::SetCursorPosY(rowStartY);
				DrawPin(node.outputs[i], contentWidth);
			}
			ImGui::PopID();
		}
	}

	void NodeEditorCanvas::DrawNodeHeader(const Node& node) {
		const ImVec2 min = ImGui::GetCursorScreenPos();
		const ImVec2 size(kStyle.nodeWidth, kStyle.headerHeight);
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(min, ImVec2(min.x + size.x, min.y + size.y), ToU32(GetNodeHeaderColor(node)), 6.0f, ImDrawFlags_RoundCornersTop);
		drawList->AddRectFilled(ImVec2(min.x, min.y + size.y - 1.0f), ImVec2(min.x + size.x, min.y + size.y), ToU32(ImVec4(1, 1, 1, 0.10f)));
		drawList->AddCircleFilled(ImVec2(min.x + 14.0f, min.y + 15.0f), 3.5f, ToU32(ImVec4(0.78f, 0.86f, 0.94f, 1.0f)), 12);
		drawList->AddText(ImVec2(min.x + 26.0f, min.y + 7.0f), ToU32(kStyle.text), node.title.c_str());
		ImGui::Dummy(size);
		ImGui::Dummy(ImVec2(kStyle.nodeWidth, 2.0f));
	}

	ImVec4 NodeEditorCanvas::GetNodeHeaderColor(const Node& node) const {
		if(node.type == "Output") return ImVec4(0.16f, 0.31f, 0.47f, 1.0f);
		if(node.type == "ToonMaster" || node.type == "LitMaster" || node.type == "UnlitMaster") return ImVec4(0.30f, 0.24f, 0.40f, 1.0f);
		if(node.type == "Color" || node.type == "MultiplyColor" || node.type == "LerpColor" || node.type == "TextureSample" || node.type == "NoiseTexture") return ImVec4(0.38f, 0.30f, 0.13f, 1.0f);
		if(node.type == "ObjectTexture") return ImVec4(0.20f, 0.32f, 0.36f, 1.0f);
		if(node.type == "LightingMode") return ImVec4(0.17f, 0.34f, 0.52f, 1.0f);
		if(node.type == "Reflect") return ImVec4(0.20f, 0.38f, 0.35f, 1.0f);
		if(node.type == "Shininess" || node.type == "Roughness" || node.type == "MultiplyFloat") return ImVec4(0.18f, 0.28f, 0.46f, 1.0f);
		return ImVec4(0.20f, 0.24f, 0.30f, 1.0f);
	}

	ImVec4 NodeEditorCanvas::GetPinColor(NodeValueType type) const {
		switch(type) {
		case NodeValueType::Float:
			return ImVec4(0.25f, 0.58f, 1.0f, 1.0f);
		case NodeValueType::Float2:
			return ImVec4(0.20f, 0.72f, 0.95f, 1.0f);
		case NodeValueType::Float3:
			return ImVec4(0.20f, 0.82f, 0.72f, 1.0f);
		case NodeValueType::Float4:
			return ImVec4(0.55f, 0.72f, 1.0f, 1.0f);
		case NodeValueType::Color:
			return ImVec4(0.95f, 0.28f, 0.24f, 1.0f);
		case NodeValueType::Bool:
			return ImVec4(0.88f, 0.20f, 0.18f, 1.0f);
		case NodeValueType::Int:
			return ImVec4(0.34f, 0.86f, 0.42f, 1.0f);
		case NodeValueType::Material:
			return ImVec4(0.84f, 0.46f, 1.0f, 1.0f);
		case NodeValueType::Texture2D:
			return ImVec4(0.22f, 0.76f, 0.82f, 1.0f);
		default:
			return ImVec4(0.82f, 0.86f, 0.90f, 1.0f);
		}
	}

	float NodeEditorCanvas::GetLinkThickness(NodeValueType type) const {
		return type == NodeValueType::Color ? kStyle.linkThickness + 0.4f : kStyle.linkThickness;
	}

	bool NodeEditorCanvas::CanCreateLink(const NodeGraph& graph, int32_t a, int32_t b, int32_t& from, int32_t& to) const {
		const Node* nodeA = nullptr;
		const Node* nodeB = nullptr;
		const NodePin* pinA = graph.FindPin(a, &nodeA);
		const NodePin* pinB = graph.FindPin(b, &nodeB);
		if(!pinA || !pinB || !nodeA || !nodeB || nodeA == nodeB) return false;
		if(pinA->kind == pinB->kind || pinA->valueType != pinB->valueType) return false;
		from = pinA->kind == NodePinKind::Output ? pinA->id : pinB->id;
		to = pinA->kind == NodePinKind::Input ? pinA->id : pinB->id;
		return true;
	}

	bool NodeEditorCanvas::Draw(NodeGraph& graph, const DrawNodeBody& drawBody, const DrawContextMenu& drawContextMenu, const GraphMutationCommand& graphMutationCommand) {
		bool changed = false;
		backgroundContextRequested_ = false;
		nodeContextRequested_ = false;
		if(lastGraph_ != &graph) {
			lastGraph_ = &graph;
			positionedNodes_.clear();
		}
		ed::SetCurrentEditor(context_);
		ed::PushStyleColor(ed::StyleColor_Bg, kStyle.bg);
		ed::PushStyleColor(ed::StyleColor_Grid, kStyle.grid);
		ed::PushStyleColor(ed::StyleColor_NodeBg, kStyle.nodeBg);
		ed::PushStyleColor(ed::StyleColor_NodeBorder, kStyle.nodeBorder);
		ed::PushStyleColor(ed::StyleColor_HovNodeBorder, kStyle.hoveredBorder);
		ed::PushStyleColor(ed::StyleColor_SelNodeBorder, kStyle.selectedBorder);
		ed::PushStyleColor(ed::StyleColor_LinkSelRectBorder, ImVec4(0.30f, 0.52f, 0.78f, 1.0f));
		ed::PushStyleColor(ed::StyleColor_PinRect, ImVec4(0.25f, 0.58f, 1.0f, 0.25f));
		ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0.0f, 0.0f, 0.0f, kStyle.bodyPaddingY));
		ed::PushStyleVar(ed::StyleVar_NodeRounding, 5.0f);
		ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 1.0f);
		ed::PushStyleVar(ed::StyleVar_HoveredNodeBorderWidth, 1.8f);
		ed::PushStyleVar(ed::StyleVar_SelectedNodeBorderWidth, 2.4f);
		ed::PushStyleVar(ed::StyleVar_LinkStrength, 95.0f);
		ed::Begin(id_.c_str());
		{
			const ImVec2 windowPos = ImGui::GetWindowPos();
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImVec2 screenCenter(windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f);
			const ImVec2 canvasCenter = ed::ScreenToCanvas(screenCenter);
			lastViewCenter_ = {canvasCenter.x, canvasCenter.y};
		}

		for(auto& node : graph.nodes) {
			ed::BeginNode(ed::NodeId(node.id));
			DrawNodeHeader(node);
			ImGui::Dummy(ImVec2(kStyle.bodyPaddingX, 0.0f));
			ImGui::SameLine();
			ImGui::BeginGroup();
			if(drawBody) changed |= drawBody(node);
			if(drawBody) ImGui::Dummy(ImVec2(1.0f, 4.0f));
			DrawNodePins(node);
			ImGui::EndGroup();
			ed::EndNode();

			if(!positionedNodes_.contains(node.id)) {
				ed::SetNodePosition(ed::NodeId(node.id), ImVec2(node.position.x, node.position.y));
				positionedNodes_.insert(node.id);
			}
			ImVec2 pos = ed::GetNodePosition(ed::NodeId(node.id));
			if(pos.x != node.position.x || pos.y != node.position.y) {
				node.position = {pos.x, pos.y};
				changed = true;
			}
		}

		for(const auto& link : graph.links) {
			const NodePin* pin = graph.FindPin(link.fromPinId);
			const NodeValueType type = pin ? pin->valueType : NodeValueType::None;
			ed::Link(ed::LinkId(link.id), ed::PinId(link.fromPinId), ed::PinId(link.toPinId), GetPinColor(type), GetLinkThickness(type));
		}

		ed::NodeId selectedNodes[1];
		selectedNodeId_ = ed::GetSelectedNodes(selectedNodes, 1) > 0 ? static_cast<int32_t>(selectedNodes[0].Get()) : 0;

		if(ed::BeginCreate()) {
			ed::PinId a, b;
			if(ed::QueryNewLink(&a, &b, ImVec4(0.70f, 0.76f, 0.84f, 1.0f), kStyle.validPreviewThickness)) {
				int32_t from = 0;
				int32_t to = 0;
				if(CanCreateLink(graph, static_cast<int32_t>(a.Get()), static_cast<int32_t>(b.Get()), from, to)) {
					const NodePin* fromPin = graph.FindPin(from);
					const ImVec4 previewColor = fromPin ? GetPinColor(fromPin->valueType) : ImVec4(0.70f, 0.76f, 0.84f, 1.0f);
					if(ed::AcceptNewItem(previewColor, kStyle.validPreviewThickness)) {
						const NodeGraph before = graph;
						graph.links.push_back({graph.AllocateId(), from, to});
						if(graphMutationCommand) graphMutationCommand("Create Node Link", before, graph);
						changed = true;
					}
				} else {
					ed::RejectNewItem(kStyle.reject, kStyle.invalidPreviewThickness);
				}
			}

			ed::PinId sourcePin;
			if(ed::QueryNewNode(&sourcePin, ImVec4(0.70f, 0.76f, 0.84f, 1.0f), kStyle.validPreviewThickness)) {
				if(ed::AcceptNewItem(ImVec4(0.70f, 0.76f, 0.84f, 1.0f), kStyle.validPreviewThickness)) {
					backgroundContextRequested_ = true;
					ImVec2 pos = ed::ScreenToCanvas(ImGui::GetMousePos());
					contextCanvasPos_ = {pos.x, pos.y};
					activeContextMenu_ = {ContextMenuType::Background, contextCanvasPos_, 0};
					hasActiveContextMenu_ = true;
				}
			}
		}
		ed::EndCreate();

		if(ed::BeginDelete()) {
			ed::NodeId deletedNode;
			while(ed::QueryDeletedNode(&deletedNode)) {
				if(ed::AcceptDeletedItem()) {
					const NodeGraph before = graph;
					const int32_t nodeId = static_cast<int32_t>(deletedNode.Get());
					std::erase_if(graph.nodes, [nodeId](const Node& n) { return n.id == nodeId; });
					std::erase_if(graph.links, [&graph](const NodeLink& l) {
						return graph.FindPin(l.fromPinId) == nullptr || graph.FindPin(l.toPinId) == nullptr;
					});
					positionedNodes_.erase(nodeId);
					if(graphMutationCommand) graphMutationCommand("Delete Node", before, graph);
					changed = true;
				}
			}

			ed::LinkId deletedLink;
			while(ed::QueryDeletedLink(&deletedLink)) {
				if(ed::AcceptDeletedItem()) {
					const NodeGraph before = graph;
					const int32_t id = static_cast<int32_t>(deletedLink.Get());
					std::erase_if(graph.links, [id](const NodeLink& l) { return l.id == id; });
					if(graphMutationCommand) graphMutationCommand("Delete Node Link", before, graph);
					changed = true;
				}
			}
		}
		ed::EndDelete();

		ed::NodeId hoveredNode;
		if(ed::ShowNodeContextMenu(&hoveredNode)) {
			nodeContextRequested_ = true;
			contextNodeId_ = static_cast<int32_t>(hoveredNode.Get());
			activeContextMenu_ = {ContextMenuType::Node, {}, contextNodeId_};
			hasActiveContextMenu_ = true;
		}
		if(ed::ShowBackgroundContextMenu()) {
			backgroundContextRequested_ = true;
			ImVec2 pos = ed::ScreenToCanvas(ImGui::GetMousePos());
			contextCanvasPos_ = {pos.x, pos.y};
			activeContextMenu_ = {ContextMenuType::Background, contextCanvasPos_, 0};
			hasActiveContextMenu_ = true;
		}

		ed::End();

		if(drawContextMenu) {
			if(hasActiveContextMenu_) {
				ImGui::OpenPopup(activeContextMenu_.type == ContextMenuType::Background ? "NodeEditorBackgroundContextMenu" : "NodeEditorNodeContextMenu");
				hasActiveContextMenu_ = false;
			}
			if(ImGui::BeginPopup("NodeEditorBackgroundContextMenu")) {
				activeContextMenu_.type = ContextMenuType::Background;
				changed |= drawContextMenu(activeContextMenu_);
				ImGui::EndPopup();
			}
			if(ImGui::BeginPopup("NodeEditorNodeContextMenu")) {
				activeContextMenu_.type = ContextMenuType::Node;
				changed |= drawContextMenu(activeContextMenu_);
				ImGui::EndPopup();
			}
		}

		ed::PopStyleVar(6);
		ed::PopStyleColor(8);
		ed::SetCurrentEditor(nullptr);
		return changed;
	}

	bool NodeEditorCanvas::ConsumeBackgroundContextRequest(Vector2& outCanvasPos) {
		if(!backgroundContextRequested_) return false;
		backgroundContextRequested_ = false;
		outCanvasPos = contextCanvasPos_;
		return true;
	}

	bool NodeEditorCanvas::ConsumeNodeContextRequest(int32_t& outNodeId) {
		if(!nodeContextRequested_) return false;
		nodeContextRequested_ = false;
		outNodeId = contextNodeId_;
		return true;
	}
}
