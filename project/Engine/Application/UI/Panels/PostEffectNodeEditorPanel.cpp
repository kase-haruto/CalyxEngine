#include "PostEffectNodeEditorPanel.h"

#include <CalyxEngine/Project.h>
#include <Engine/PostProcess/Manager/PostEffectManager.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>
#include <Engine/System/Command/EditorCommand/ValueEditCommand.h>
#include <Engine/System/Command/Manager/CommandManager.h>
#include <externals/imgui/imgui.h>

#include <algorithm>
#include <cstring>
#include <fstream>

namespace CalyxEngine {
	namespace {
		constexpr const char* kDefaultPath = "PostEffects/Default.postfx";
		constexpr const char* kEffectTypes[] = {
			"RadialBlur",
			"ChromaticAberration",
			"Vignette",
			"CRTEffect",
			"Bloom",
			"Blend"
		};
		constexpr const char* kFloatParams[] = {
			"width",
			"intensity",
			"threshold",
			"softKnee",
			"strength",
			"radius",
			"center.x",
			"center.y",
			"color.r",
			"color.g",
			"color.b",
			"tint.r",
			"tint.g",
			"tint.b",
			"opacity",
			"mode"
		};

		nlohmann::json DefaultParameters(const std::string& type) {
			if(type == "RadialBlur") return {{"center", {0.5f, 0.5f}}, {"width", 0.0f}};
			if(type == "ChromaticAberration") return {{"intensity", 0.0f}};
			if(type == "Vignette") return {{"strength", 1.0f}, {"radius", 0.0f}, {"color", {0.0f, 0.0f, 0.0f}}};
			if(type == "CRTEffect") return {{"screenSize", {1280.0f, 720.0f}}};
			if(type == "Bloom") return {{"intensity", 0.7f}, {"threshold", 0.8f}, {"softKnee", 0.5f}, {"radius", 1.0f}, {"tint", {1.0f, 1.0f, 1.0f}}};
			if(type == "Blend") return {{"opacity", 0.5f}, {"mode", 0}};
			return nlohmann::json::object();
		}

		nlohmann::json& EnsureObject(nlohmann::json& root, const char* key) {
			if(!root.contains(key) || !root[key].is_object()) root[key] = nlohmann::json::object();
			return root[key];
		}

		nlohmann::json& EnsureArray(nlohmann::json& root, const char* key) {
			if(!root.contains(key) || !root[key].is_array()) root[key] = nlohmann::json::array();
			return root[key];
		}

		void HelpTooltip(const char* text) {
			if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("%s", text);
			}
		}

		bool IsAnimatableParameter(const std::string& nodeType, const char* parameter) {
			if(nodeType == "RadialBlur") {
				return std::string(parameter) == "width" ||
					   std::string(parameter) == "center.x" ||
					   std::string(parameter) == "center.y";
			}
			if(nodeType == "ChromaticAberration") {
				return std::string(parameter) == "intensity";
			}
			if(nodeType == "Vignette") {
				return std::string(parameter) == "strength" ||
					   std::string(parameter) == "radius" ||
					   std::string(parameter) == "color.r" ||
					   std::string(parameter) == "color.g" ||
					   std::string(parameter) == "color.b";
			}
			if(nodeType == "Bloom") {
				const std::string name(parameter);
				return name == "intensity" ||
					   name == "threshold" ||
					   name == "softKnee" ||
					   name == "radius" ||
					   name == "tint.r" ||
					   name == "tint.g" ||
					   name == "tint.b";
			}
			if(nodeType == "Blend") {
				return std::string(parameter) == "opacity";
			}
			return false;
		}
	}

	PostEffectNodeEditorPanel::PostEffectNodeEditorPanel()
		: IEngineUI("Post Effect Graph"), canvas_("PostEffectGraphCanvas") {
		isShow_ = false;
		const std::string defaultPath = Calyx::ResolveAssetPath(kDefaultPath).generic_string();
		std::copy_n(defaultPath.c_str(), (std::min)(defaultPath.size(), pathBuffer_.size() - 1), pathBuffer_.data());
		EnsureIoNodes();
	}

	void PostEffectNodeEditorPanel::Render() {
		if(!IsShow()) return;

		bool open = true;
		// CanvasとInspectorを別領域に固定し、Node操作中にPanel全体がScrollするのを防ぐ。
		if(ImGui::Begin(panelName_.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			DrawToolbar();
			EnsureIoNodes();
			ImGui::BeginChild("##post-effect-graph-canvas", ImVec2(-330.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			bool pendingGraphCommand = false;
			std::string pendingGraphCommandName;
			NodeGraph pendingGraphBefore;
			NodeGraph pendingGraphAfter;
			// Canvasから返る変更前後のSnapshotを集約し、一操作を一つのUndo Commandとして記録する。
			const bool graphChanged = canvas_.Draw(
				graph_,
				[this, &pendingGraphCommand, &pendingGraphCommandName, &pendingGraphBefore, &pendingGraphAfter](Node& node) {
					const NodeGraph before = graph_;
					const bool changed = DrawNodeBody(node);
					if(changed) {
						// Drag中は開始時Snapshotだけを保持し、毎フレームUndo履歴を増やさない。
						if(ImGui::IsAnyItemActive()) {
							if(!nodeEditCommandActive_) {
								nodeEditCommandActive_ = true;
								nodeEditBefore_ = before;
							}
						} else {
							pendingGraphCommand = true;
							pendingGraphCommandName = "Edit Post Effect Node";
							pendingGraphBefore = before;
							pendingGraphAfter = graph_;
						}
					}
					return changed;
				},
				[this](const NodeEditorCanvas::ContextMenu& menu) { return DrawContextMenu(menu); },
				[&pendingGraphCommand, &pendingGraphCommandName, &pendingGraphBefore, &pendingGraphAfter](const char* name, const NodeGraph& before, const NodeGraph& after) {
					pendingGraphCommand = true;
					pendingGraphCommandName = name;
					pendingGraphBefore = before;
					pendingGraphAfter = after;
				});
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::BeginChild("##post-effect-graph-inspector", ImVec2(0.0f, 0.0f), true);
			DrawInspector();
			ImGui::EndChild();

			if(graphChanged) {
			}
			// ImGui Itemが非Activeになった時点を編集確定としてCommandを発行する。
			if(nodeEditCommandActive_ && !ImGui::IsAnyItemActive()) {
				ExecuteGraphCommand("Edit Post Effect Node", nodeEditBefore_, graph_);
				nodeEditCommandActive_ = false;
			}
			if(pendingGraphCommand) {
				ExecuteGraphCommand(pendingGraphCommandName.c_str(), pendingGraphBefore, pendingGraphAfter);
			}
		}
		ImGui::End();
		if(!open) SetShow(false);
	}

	void PostEffectNodeEditorPanel::DrawToolbar() {
		// 保存・読込・Runtime適用を同じAsset Pathに対して実行し、PreviewとDisk内容を一致させる。
		ImGui::SetNextItemWidth(360.0f);
		ImGui::InputText("Path", pathBuffer_.data(), pathBuffer_.size());
		ImGui::SameLine();
		if(ImGui::Button("+ Add Node", ImVec2(104.0f, 0.0f))) ImGui::OpenPopup("PostEffectAddNodeToolbar");
		if(ImGui::BeginPopup("PostEffectAddNodeToolbar")) {
			DrawAddNodeMenu(canvas_.GetLastViewCenter());
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Save", ImVec2(70.0f, 0.0f))) Save();
		ImGui::SameLine();
		if(ImGui::Button("Load", ImVec2(70.0f, 0.0f))) Load();
		ImGui::SameLine();
		if(ImGui::Button("Apply", ImVec2(70.0f, 0.0f))) Apply();
		ImGui::SameLine();
		if(ImGui::Button("Play", ImVec2(70.0f, 0.0f))) {
			// 編集中Graphを先にRuntimeへ適用してからTriggered Effectを開始する。
			Apply();
			PostEffectManager::Get()->PlayTriggeredEffects();
		}
		ImGui::SameLine();
		if(ImGui::Checkbox("Outline", &outlineEnabled_)) {
			PostEffectManager::Get()->SetOutlineEnabled(outlineEnabled_);
		}
		HelpTooltip("Controls the scene outline pass. This setting is saved into the post-effect preset.");
		ImGui::Separator();
	}

	bool PostEffectNodeEditorPanel::DrawContextMenu(const NodeEditorCanvas::ContextMenu& menu) {
		if(menu.type == NodeEditorCanvas::ContextMenuType::Background) {
			return DrawAddNodeMenu(menu.canvasPosition);
		}
		ImGui::TextDisabled("Delete: select node and press Delete");
		return false;
	}

	bool PostEffectNodeEditorPanel::DrawAddNodeMenu(Vector2 position) {
		const NodeGraph before = graph_;
		bool changed = false;
		for(const char* type : kEffectTypes) {
			if(ImGui::MenuItem(type)) {
				AddEffectNode(type, position);
				changed = true;
			}
		}
		if(changed) {
			ExecuteGraphCommand("Add Post Effect Node", before, graph_);
		}
		return changed;
	}

	void PostEffectNodeEditorPanel::ExecuteGraphCommand(const char* name, const NodeGraph& before, const NodeGraph& after) {
		auto apply = [this](const NodeGraph& graph) {
			// Undo/Redo後にID採番と必須IO Nodeを再検証し、次の編集でID衝突を起こさない。
			graph_ = graph;
			graph_.EnsureNextId();
			EnsureIoNodes();
		};
		CommandManager::GetInstance()->Execute(
			std::make_unique<ValueEditCommand<NodeGraph>>(name, before, after, apply));
	}

	bool PostEffectNodeEditorPanel::DrawNodeBody(Node& node) {
		if(node.type == "Input") {
			ImGui::TextUnformatted("Scene Color");
			return false;
		}
		if(node.type == "Output") {
			ImGui::TextUnformatted("Final Color");
			return false;
		}

		bool changed = false;
		auto& props = node.properties;
		auto& params = EnsureObject(props, "parameters");

		const std::string applyModeText = props.value("applyMode", std::string("Always"));
		if(applyModeText == "Always") {
			bool enabled = props.value("enabled", true);
			if(ImGui::Checkbox("Enabled", &enabled)) {
				props["enabled"] = enabled;
				changed = true;
			}
			ImGui::SameLine();
		}
		ImGui::TextDisabled("%s", applyModeText.c_str());

		if(node.type == "RadialBlur") {
			ImGui::TextDisabled("width %.2f", params.value("width", 0.0f));
		} else if(node.type == "ChromaticAberration") {
			ImGui::TextDisabled("intensity %.2f", params.value("intensity", 0.0f));
		} else if(node.type == "Vignette") {
			ImGui::TextDisabled("strength %.2f", params.value("strength", 1.0f));
		} else if(node.type == "Bloom") {
			ImGui::TextDisabled("intensity %.2f", params.value("intensity", 0.7f));
		} else if(node.type == "Blend") {
			ImGui::TextDisabled("opacity %.2f", params.value("opacity", 0.5f));
		}

		ImGui::TextDisabled("Select node to edit");

		return changed;
	}

	void PostEffectNodeEditorPanel::DrawInspector() {
		ImGui::TextUnformatted("Inspector");
		ImGui::Separator();

		const int32_t selectedId = canvas_.GetSelectedNodeId();
		Node* selected = graph_.FindNode(selectedId);
		if(!selected) {
			ImGui::TextDisabled("Select a node.");
			return;
		}

		if(DrawNodeInspector(*selected)) {
		}
	}

	bool PostEffectNodeEditorPanel::DrawNodeInspector(Node& node) {
		ImGui::TextUnformatted(node.title.c_str());
		ImGui::TextDisabled("%s", node.type.c_str());
		ImGui::Separator();

		if(node.type == "Input" || node.type == "Output") {
			ImGui::TextDisabled("No parameters.");
			return false;
		}

		bool changed = false;
		auto& props = node.properties;
		auto& params = EnsureObject(props, "parameters");
		auto& animations = EnsureArray(props, "floatAnimations");

		int applyMode = props.value("applyMode", std::string("Always")) == "Triggered" ? 1 : 0;
		if(ImGui::Combo("Apply Mode", &applyMode, "Always\0Triggered\0")) {
			props["applyMode"] = applyMode == 1 ? "Triggered" : "Always";
			if(applyMode == 1) props["enabled"] = false;
			changed = true;
		}
		HelpTooltip("Always: the effect is applied every frame. Triggered: the effect is normally off and plays for a duration when Play is called.");

		const bool isTriggered = applyMode == 1;
		if(!isTriggered) {
			bool enabled = props.value("enabled", true);
			if(ImGui::Checkbox("Enabled", &enabled)) {
				props["enabled"] = enabled;
				changed = true;
			}
			HelpTooltip("Controls whether this Always effect is included in the post-effect graph.");
		} else {
			props["enabled"] = false;
			ImGui::TextDisabled("Triggered nodes are enabled only while playing.");
			if(ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
				ImGui::SetTooltip("Use Play to enable this node temporarily. It will be disabled again when the animation finishes.");
			}
		}

		if(isTriggered) {
			float duration = props.value("duration", 0.25f);
			if(ImGui::DragFloat("Duration", &duration, 0.01f, 0.0f, 60.0f, "%.2f sec")) {
				props["duration"] = duration;
				changed = true;
			}
			HelpTooltip("How long the triggered animation runs after Play is called.");

			int ease = props.value("easeIndex", static_cast<int>(EaseType::EaseOutSine));
			if(SelectEaseInt("Ease", ease)) {
				props["easeIndex"] = ease;
				changed = true;
			}
			HelpTooltip("Curve used to interpolate animated parameters from From to To.");

			bool autoDisable = props.value("autoDisable", true);
			if(ImGui::Checkbox("Auto Disable", &autoDisable)) {
				props["autoDisable"] = autoDisable;
				changed = true;
			}
			HelpTooltip("Turns the effect off automatically when the triggered animation finishes.");
		}

		ImGui::SeparatorText("Parameters");
		if(node.type == "RadialBlur") {
			float center[2] = {0.5f, 0.5f};
			if(params.contains("center") && params["center"].is_array() && params["center"].size() == 2) {
				center[0] = params["center"][0].get<float>();
				center[1] = params["center"][1].get<float>();
			}
			float width = params.value("width", 0.0f);
			if(ImGui::DragFloat2("Center", center, 0.01f, 0.0f, 1.0f)) {
				params["center"] = {center[0], center[1]};
				changed = true;
			}
			HelpTooltip("UV center of the radial blur. 0.5, 0.5 is the screen center.");
			if(ImGui::DragFloat("Width", &width, 0.01f, 0.0f, 2.0f)) {
				params["width"] = width;
				changed = true;
			}
			HelpTooltip("Radial blur strength. 0 means no visible blur.");
		} else if(node.type == "ChromaticAberration") {
			float intensity = params.value("intensity", 0.0f);
			if(ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 2.0f)) {
				params["intensity"] = intensity;
				changed = true;
			}
			HelpTooltip("Color separation strength. 0 means no visible chromatic aberration.");
		} else if(node.type == "Vignette") {
			float strength = params.value("strength", 1.0f);
			float radius = params.value("radius", 0.0f);
			float color[3] = {0.0f, 0.0f, 0.0f};
			if(params.contains("color") && params["color"].is_array() && params["color"].size() >= 3) {
				color[0] = params["color"][0].get<float>();
				color[1] = params["color"][1].get<float>();
				color[2] = params["color"][2].get<float>();
			}
			if(ImGui::DragFloat("Strength", &strength, 0.01f, 0.0f, 1.0f)) {
				params["strength"] = strength;
				changed = true;
			}
			HelpTooltip("Darkening strength around the screen edges.");
			if(ImGui::DragFloat("Radius", &radius, 0.01f, 0.0f, 1.0f)) {
				params["radius"] = radius;
				changed = true;
			}
			HelpTooltip("Area before the vignette starts. Larger values keep more of the center unaffected.");
			if(ImGui::ColorEdit3("Color", color)) {
				params["color"] = {color[0], color[1], color[2]};
				changed = true;
			}
			HelpTooltip("Color blended into the vignette around the screen edges.");
		} else if(node.type == "Bloom") {
			float intensity = params.value("intensity", 0.7f);
			float threshold = params.value("threshold", 0.8f);
			float softKnee = params.value("softKnee", 0.5f);
			float radius = params.value("radius", 1.0f);
			float tint[3] = {1.0f, 1.0f, 1.0f};
			if(params.contains("tint") && params["tint"].is_array() && params["tint"].size() == 3) {
				tint[0] = params["tint"][0].get<float>();
				tint[1] = params["tint"][1].get<float>();
				tint[2] = params["tint"][2].get<float>();
			}
			if(ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 5.0f)) {
				params["intensity"] = intensity;
				changed = true;
			}
			HelpTooltip("Brightness added by the bloom pass.");
			if(ImGui::DragFloat("Threshold", &threshold, 0.01f, -1.0f, 5.0f)) {
				params["threshold"] = threshold;
				changed = true;
			}
			HelpTooltip("Minimum source brightness that starts blooming. Lower values bloom more pixels.");
			if(ImGui::DragFloat("Soft Knee", &softKnee, 0.01f, 0.0f, 2.0f)) {
				params["softKnee"] = softKnee;
				changed = true;
			}
			HelpTooltip("Soft transition around the threshold.");
			if(ImGui::DragFloat("Radius", &radius, 0.01f, 0.0f, 4.0f)) {
				params["radius"] = radius;
				changed = true;
			}
			HelpTooltip("Spread of the bloom blur.");
			if(ImGui::ColorEdit3("Tint", tint)) {
				params["tint"] = {tint[0], tint[1], tint[2]};
				changed = true;
			}
			HelpTooltip("Color multiplier for the bloom contribution.");
		} else if(node.type == "Blend") {
			float opacity = params.value("opacity", 0.5f);
			int blendMode = params.value("mode", 0);
			if(ImGui::DragFloat("Opacity", &opacity, 0.01f, 0.0f, 1.0f)) {
				params["opacity"] = opacity;
				changed = true;
			}
			HelpTooltip("Blend amount. In Lerp mode, 0 shows the previous input and 1 shows the next input.");
			if(ImGui::Combo("Mode", &blendMode, "Lerp\0Add\0Multiply\0Max\0")) {
				params["mode"] = blendMode;
				changed = true;
			}
			HelpTooltip("How input A and input B are combined.");
			if(blendMode == 0 && opacity >= 0.999f) {
				ImGui::TextColored(ImVec4(1.0f, 0.72f, 0.30f, 1.0f), "Lerp opacity 1.0 shows only the later input.");
			}
			changed |= DrawBlendInputControls(node);
		}

		if(isTriggered) {
			ImGui::SeparatorText("Float Animations");
			if(ImGui::BeginCombo("Add Animated Parameter", "select")) {
				for(const char* param : kFloatParams) {
					if(!IsAnimatableParameter(node.type, param)) continue;
					if(ImGui::Selectable(param)) {
						animations.push_back({{"parameter", param}, {"from", 0.0f}, {"to", 0.0f}, {"useCurrentAsFrom", false}});
						changed = true;
					}
				}
				ImGui::EndCombo();
			}
			HelpTooltip("Adds a float parameter that will be animated when this Triggered node is played.");
			for(int i = 0; i < static_cast<int>(animations.size()); ++i) {
				auto& anim = animations[i];
				ImGui::PushID(i);
				ImGui::TextUnformatted(anim.value("parameter", "").c_str());
				ImGui::SameLine();
				if(ImGui::SmallButton("Remove")) {
					animations.erase(animations.begin() + i);
					ImGui::PopID();
					--i;
					changed = true;
					continue;
				}
				bool useCurrent = anim.value("useCurrentAsFrom", false);
				if(ImGui::Checkbox("Use Current As From", &useCurrent)) {
					anim["useCurrentAsFrom"] = useCurrent;
					changed = true;
				}
				HelpTooltip("Starts from the effect's current runtime value instead of the From value below.");
				float from = anim.value("from", 0.0f);
				float to = anim.value("to", 0.0f);
				if(!useCurrent && ImGui::DragFloat("From", &from, 0.01f)) {
					anim["from"] = from;
					changed = true;
				}
				if(!useCurrent) HelpTooltip("Value at the moment Play starts.");
				if(ImGui::DragFloat("To", &to, 0.01f)) {
					anim["to"] = to;
					changed = true;
				}
				HelpTooltip("Value reached at the end of Duration.");
				ImGui::PopID();
			}
		}

		return changed;
	}

	bool PostEffectNodeEditorPanel::DrawBlendInputControls(Node& node) {
		if(node.type != "Blend") return false;

		bool changed = false;
		ImGui::SeparatorText("Inputs");
		ImGui::TextDisabled("%d color inputs", static_cast<int>(node.inputs.size()));
		HelpTooltip("Blend combines all connected inputs in order from top to bottom.");

		if(ImGui::Button("+ Input", ImVec2(92.0f, 0.0f))) {
			const int32_t index = static_cast<int32_t>(node.inputs.size());
			const char label = static_cast<char>('A' + (std::min)(index, 25));
			node.inputs.push_back({graph_.AllocateId(), std::string(1, label), NodePinKind::Input, NodePinTypes::Color});
			node.properties["inputCount"] = static_cast<int>(node.inputs.size());
			changed = true;
		}
		ImGui::SameLine();
		const bool canRemove = node.inputs.size() > 2;
		if(!canRemove) ImGui::BeginDisabled();
		if(ImGui::Button("- Input", ImVec2(92.0f, 0.0f))) {
			const int32_t removedPinId = node.inputs.back().id;
			node.inputs.pop_back();
			std::erase_if(graph_.links, [removedPinId](const NodeLink& link) {
				return link.toPinId == removedPinId || link.fromPinId == removedPinId;
			});
			node.properties["inputCount"] = static_cast<int>(node.inputs.size());
			changed = true;
		}
		if(!canRemove) ImGui::EndDisabled();
		HelpTooltip("Blend keeps at least two inputs. Removing an input also removes its links.");

		for(int32_t i = 0; i < static_cast<int32_t>(node.inputs.size()); ++i) {
			const char label = static_cast<char>('A' + (std::min)(i, 25));
			const std::string expectedName = std::string(1, label);
			if(node.inputs[i].name != expectedName) {
				node.inputs[i].name = expectedName;
				changed = true;
			}
		}

		return changed;
	}

	void PostEffectNodeEditorPanel::AddInputNode(Vector2 position) {
		Node node;
		node.id = graph_.AllocateId();
		node.type = "Input";
		node.title = "Scene Color";
		node.position = position;
		node.outputs.push_back({graph_.AllocateId(), "Color", NodePinKind::Output, NodePinTypes::Color});
		graph_.nodes.push_back(std::move(node));
	}

	void PostEffectNodeEditorPanel::AddOutputNode(Vector2 position) {
		Node node;
		node.id = graph_.AllocateId();
		node.type = "Output";
		node.title = "Output";
		node.position = position;
		node.inputs.push_back({graph_.AllocateId(), "Color", NodePinKind::Input, NodePinTypes::Color});
		graph_.nodes.push_back(std::move(node));
	}

	void PostEffectNodeEditorPanel::AddEffectNode(const std::string& type, Vector2 position) {
		Node node;
		node.id = graph_.AllocateId();
		node.type = type;
		node.title = type;
		node.position = position;
		if(type == "Blend") {
			node.inputs.push_back({graph_.AllocateId(), "A", NodePinKind::Input, NodePinTypes::Color});
			node.inputs.push_back({graph_.AllocateId(), "B", NodePinKind::Input, NodePinTypes::Color});
		}else{
			node.inputs.push_back({graph_.AllocateId(), "In", NodePinKind::Input, NodePinTypes::Color});
		}
		node.outputs.push_back({graph_.AllocateId(), "Out", NodePinKind::Output, NodePinTypes::Color});
		node.properties = {
			{"enabled", true},
			{"applyMode", "Always"},
			{"duration", 0.25f},
			{"easeIndex", static_cast<int>(EaseType::EaseOutSine)},
			{"autoDisable", true},
			{"parameters", DefaultParameters(type)},
			{"floatAnimations", nlohmann::json::array()}
		};
		if(type == "Blend") {
			node.properties["inputCount"] = static_cast<int>(node.inputs.size());
		}
		graph_.nodes.push_back(std::move(node));
	}

	void PostEffectNodeEditorPanel::EnsureIoNodes() {
		const bool hasInput = std::any_of(graph_.nodes.begin(), graph_.nodes.end(), [](const Node& node) { return node.type == "Input"; });
		const bool hasOutput = std::any_of(graph_.nodes.begin(), graph_.nodes.end(), [](const Node& node) { return node.type == "Output"; });
		// Runtime Graphが必ず入口と出口を持つよう、不足している場合だけ補完する。
		if(!hasInput) AddInputNode({20.0f, 120.0f});
		if(!hasOutput) AddOutputNode({720.0f, 120.0f});
	}

	std::vector<const Node*> PostEffectNodeEditorPanel::GetExecutionNodes() const {
		std::vector<const Node*> ordered;
		for(const auto& node : graph_.nodes) {
			if(node.type == "Input" || node.type == "Output") continue;
			ordered.push_back(&node);
		}
		// 旧Linear Presetとの互換用に、Canvas上の左から右を実行順として安定化する。
		std::sort(ordered.begin(), ordered.end(), [](const Node* a, const Node* b) {
			if(a->position.x == b->position.x) return a->id < b->id;
			return a->position.x < b->position.x;
		});
		return ordered;
	}

	nlohmann::json PostEffectNodeEditorPanel::BuildPresetJson() const {
		// Editor用Graph表現と、旧Runtimeが読む平坦なnodes配列の両方を保存する。
		nlohmann::json root;
		root["type"] = "PostEffectPreset";
		root["name"] = std::filesystem::path(pathBuffer_.data()).stem().string();
		root["version"] = 1;
		root["outline"] = {
			{"enabled", outlineEnabled_}
		};
		root["graph"] = graph_;
		root["nodes"] = nlohmann::json::array();

		for(const Node* node : GetExecutionNodes()) {
			nlohmann::json props = node->properties;
			root["nodes"].push_back({
				{"id", node->id},
				{"type", node->type},
				{"title", node->title},
				{"enabled", props.value("enabled", true)},
				{"applyMode", props.value("applyMode", std::string("Always"))},
				{"duration", props.value("duration", 0.25f)},
				{"ease", props.value("ease", std::string(""))},
				{"easeIndex", props.value("easeIndex", static_cast<int>(EaseType::EaseOutSine))},
				{"autoDisable", props.value("autoDisable", true)},
				{"parameters", props.value("parameters", nlohmann::json::object())},
				{"floatAnimations", props.value("floatAnimations", nlohmann::json::array())}
			});
		}
		return root;
	}

	void PostEffectNodeEditorPanel::LoadPresetJson(const nlohmann::json& root) {
		if(root.contains("outline") && root["outline"].is_object()) {
			outlineEnabled_ = root["outline"].value("enabled", outlineEnabled_);
		}

		if(root.contains("graph")) {
			// 新形式はPinとLinkを含むGraphをそのまま復元する。
			graph_ = root.at("graph").get<NodeGraph>();
			EnsureIoNodes();
			return;
		}

		// 旧形式はnodes配列しか持たないため、直列配置のGraphへ変換して読み込む。
		graph_ = {};
		AddInputNode({20.0f, 120.0f});
		float x = 220.0f;
		if(root.contains("nodes") && root["nodes"].is_array()) {
			for(const auto& item : root["nodes"]) {
				const std::string type = item.value("type", "");
				if(type.empty()) continue;
				AddEffectNode(type, {x, 120.0f});
				auto& node = graph_.nodes.back();
				node.properties["enabled"] = item.value("enabled", true);
				node.properties["applyMode"] = item.value("applyMode", std::string("Always"));
				node.properties["duration"] = item.value("duration", 0.25f);
				node.properties["easeIndex"] = item.value("easeIndex", static_cast<int>(EaseType::EaseOutSine));
				node.properties["autoDisable"] = item.value("autoDisable", true);
				node.properties["parameters"] = item.value("parameters", DefaultParameters(type));
				node.properties["floatAnimations"] = item.value("floatAnimations", nlohmann::json::array());
				x += 240.0f;
			}
		}
		AddOutputNode({x + 160.0f, 120.0f});
	}

	void PostEffectNodeEditorPanel::Save() {
		try {
			// Asset root基準へ解決し、初回保存でも書き込めるよう親Directoryを準備する。
			std::filesystem::path path = Calyx::ResolveAssetPath(pathBuffer_.data());
			FileSystemHelper::CreateDirectoryPath(path.parent_path().string());
			std::ofstream ofs(path);
			if(!ofs) return;
			ofs << BuildPresetJson().dump(2);
		} catch(...) {
		}
	}

	void PostEffectNodeEditorPanel::Load() {
		try {
			// JSON解析に成功した場合だけ現在Graphを置換し、破損Assetで編集内容を失わない。
			std::ifstream ifs(Calyx::ResolveAssetPath(pathBuffer_.data()));
			if(!ifs) return;
			nlohmann::json root;
			ifs >> root;
			LoadPresetJson(root);
		} catch(...) {
		}
	}

	void PostEffectNodeEditorPanel::Apply() {
		// 保存したPresetをManagerへ再読込させ、Editor表示とRuntime描画を同期する。
		Save();
		PostEffectManager::Get()->LoadPreset(Calyx::ResolveAssetPath(pathBuffer_.data()).generic_string());
	}
}
