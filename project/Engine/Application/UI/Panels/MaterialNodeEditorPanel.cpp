#include "MaterialNodeEditorPanel.h"

#include <Engine\Assets\DataAsset\MaterialAsset.h>
#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\Manager\AssetManager.h>
#include <Engine\Assets\Texture\TextureManager.h>
#include <Engine\Application\UI\Panels\AssetPanel.h>
#include <Engine\Foundation\Clock\ClockManager.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine\Graphics\Context\GraphicsGroup.h>
#include <Engine\Graphics\Descriptor\DescriptorAllocator.h>
#include <Engine\Graphics\MaterialGraph\MaterialGraphCompiler.h>
#include <Engine\Graphics\MaterialGraph\ShaderGraphCodeGenerator.h>
#include <Engine\Graphics\MaterialGraph\ShaderGraphSchema.h>
#include <Engine\Graphics\MaterialGraph\ShaderGraphValidator.h>
#include <Engine\Graphics\Pipeline\Factory\PsoFactory.h>
#include <Engine\Graphics\Pipeline\Shader\ShaderCompiler.h>
#include <Engine\System\Command\EditorCommand\ValueEditCommand.h>
#include <Engine\System\Command\Manager\CommandManager.h>
#include <externals\imgui\imgui.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <vector>

namespace CalyxEngine {
	namespace {
		constexpr const char* kLightingModes[] = {
			"Half-Lambert",
			"Lambert",
			"Toon",
			"No Lighting",
			"Unlit Color"};
		constexpr int32_t kLightingModeCount = static_cast<int32_t>(std::size(kLightingModes));

		bool IsObsoleteToonOutputPin(const std::string& name) {
			return name == "Toon Highlight" ||
				   name == "Toon Base" ||
				   name == "Toon Mid Shadow" ||
				   name == "Toon Shadow" ||
				   name == "Toon Threshold 1" ||
				   name == "Toon Threshold 2" ||
				   name == "Toon Threshold 3" ||
				   name == "Toon Edge Softness" ||
				   name == "Toon Spec Threshold" ||
				   name == "Toon Spec Softness" ||
				   name == "Toon Spec Intensity";
		}

		bool IsLegacyOutputPin(const std::string& name) {
			return name == "BaseColor" ||
				   name == "Emissive" ||
				   name == "Emissive Intensity" ||
				   name == "Shininess" ||
				   name == "Roughness" ||
				   name == "Reflect" ||
				   name == "Lighting Mode" ||
				   IsObsoleteToonOutputPin(name);
		}

		bool IsLightingNode(const std::string& type) {
			return type == "HalfLambertLighting" ||
				   type == "LambertLighting" ||
				   type == "ToonLighting" ||
				   type == "NoLighting" ||
				   type == "UnlitColorLighting";
		}

		int32_t LightingModeFromNodeType(const std::string& type, int32_t fallback) {
			if(type == "HalfLambertLighting") return 0;
			if(type == "LambertLighting") return 1;
			if(type == "ToonLighting") return 2;
			if(type == "NoLighting") return 3;
			if(type == "UnlitColorLighting") return 4;
			return fallback;
		}

		const NodePin* FindInputPin(const Node& node, const char* name) {
			for(const auto& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		bool IsInputLinked(const MaterialAsset& material, const Node& node, const char* name) {
			const NodePin* pin = FindInputPin(node, name);
			if(!pin) return false;
			return std::any_of(material.graph.links.begin(), material.graph.links.end(), [pin](const NodeLink& link) {
				return link.toPinId == pin->id;
			});
		}

		const Node* FindLinkedNode(const MaterialAsset& material, const Node& node, const char* inputName) {
			const NodePin* pin = FindInputPin(node, inputName);
			if(!pin) return nullptr;
			for(const NodeLink& link : material.graph.links) {
				if(link.toPinId != pin->id) continue;
				const NodePin* fromPin = material.graph.FindPin(link.fromPinId);
				if(!fromPin) return nullptr;
				for(const Node& candidate : material.graph.nodes) {
					const auto ownsOutput = std::any_of(candidate.outputs.begin(), candidate.outputs.end(), [fromPin](const NodePin& output) {
						return output.id == fromPin->id;
					});
					if(ownsOutput) return &candidate;
				}
			}
			return nullptr;
		}

		void EnsureSchemaPins(MaterialAsset& material, Node& node, const ShaderGraphNodeSchema& schemaNode) {
			for(const auto& pinSchema : schemaNode.pins) {
				auto& pins = pinSchema.kind == NodePinKind::Input ? node.inputs : node.outputs;
				const auto exists = std::any_of(pins.begin(), pins.end(), [&pinSchema](const NodePin& pin) {
					return pin.name == pinSchema.name;
				});
				if(exists) continue;
				pins.push_back({
					material.graph.AllocateId(),
					pinSchema.name,
					pinSchema.kind,
					pinSchema.type});
			}
		}

		float GetFloatProperty(const Node& node, const char* key, float fallback) {
			if(!node.properties.contains(key)) return fallback;
			return node.properties.value(key, fallback);
		}

		void SetFloatProperty(Node& node, const char* key, float value) {
			node.properties[key] = value;
		}

		Vector4 GetColorProperty(const Node& node, const char* key, const Vector4& fallback) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_array() || it->size() != 4) return fallback;
			return {it->at(0).get<float>(), it->at(1).get<float>(), it->at(2).get<float>(), it->at(3).get<float>()};
		}

		void SetColorProperty(Node& node, const char* key, const Vector4& value) {
			node.properties[key] = {value.x, value.y, value.z, value.w};
		}

		void SetDefaultToonProperties(Node& node) {
			SetColorProperty(node, "toonHighlightColor", {1.15f, 1.10f, 1.00f, 1.0f});
			SetColorProperty(node, "toonBaseColor", {1.0f, 1.0f, 1.0f, 1.0f});
			SetColorProperty(node, "toonMidShadowColor", {0.72f, 0.76f, 0.86f, 1.0f});
			SetColorProperty(node, "toonShadowColor", {0.42f, 0.46f, 0.58f, 1.0f});
			SetFloatProperty(node, "toonThreshold1", -0.15f);
			SetFloatProperty(node, "toonThreshold2", 0.25f);
			SetFloatProperty(node, "toonThreshold3", 0.82f);
			SetFloatProperty(node, "toonEdgeSoftness", 0.03f);
			SetFloatProperty(node, "toonSpecularThreshold", 0.96f);
			SetFloatProperty(node, "toonSpecularSoftness", 0.02f);
			SetFloatProperty(node, "toonSpecularIntensity", 0.35f);
		}

		void SetDefaultToonMasterProperties(Node& node) {
			SetColorProperty(node, "baseColor", {1, 1, 1, 1});
			SetColorProperty(node, "emissiveColor", {0, 0, 0, 1});
			SetFloatProperty(node, "emissiveIntensity", 0.0f);
			SetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			SetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			SetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
			SetFloatProperty(node, "baseStep", 0.25f);
			SetFloatProperty(node, "baseFeather", 0.03f);
			SetFloatProperty(node, "shadeStep", -0.15f);
			SetFloatProperty(node, "shadeFeather", 0.03f);
			SetFloatProperty(node, "specularThreshold", 0.96f);
			SetFloatProperty(node, "specularSoftness", 0.02f);
			SetFloatProperty(node, "specularIntensity", 0.35f);
		}

		const AssetRecord* FindTextureRecord(const Guid& guid) {
			if(!guid.isValid()) return nullptr;
			for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
				if(rec && rec->type == AssetType::Texture && rec->guid == guid) return rec;
			}
			return nullptr;
		}

		std::string TextureLabelFromGuid(const Guid& guid) {
			if(const AssetRecord* rec = FindTextureRecord(guid)) {
				return rec->sourcePath.filename().string();
			}
			return guid.isValid() ? "(missing texture)" : "(object/model texture)";
		}

		bool DrawTextureAssetSelector(const char* label, Guid& guid) {
			bool changed = false;
			const std::string current = TextureLabelFromGuid(guid);

			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::BeginCombo(label, current.c_str())) {
				const bool noneSelected = !guid.isValid();
				if(ImGui::Selectable("(object/model texture)", noneSelected)) {
					guid = Guid::Empty();
					changed = true;
				}
				if(noneSelected) ImGui::SetItemDefaultFocus();

				for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
					if(!rec || rec->type != AssetType::Texture) continue;
					const bool selected = guid == rec->guid;
					const std::string itemLabel = rec->sourcePath.filename().string();
					if(ImGui::Selectable(itemLabel.c_str(), selected)) {
						guid = rec->guid;
						changed = true;
					}
					if(selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			return changed;
		}

		Guid GetGuidProperty(const Node& node, const char* key) {
			auto it = node.properties.find(key);
			if(it == node.properties.end() || !it->is_string()) return Guid::Empty();
			return Guid::FromString(it->get<std::string>());
		}

		void SetGuidProperty(Node& node, const char* key, const Guid& guid) {
			if(guid.isValid()) {
				node.properties[key] = guid.ToString();
			} else {
				node.properties.erase(key);
			}
		}

		bool IsGraphTextureNodeType(const std::string& type) {
			return type == "ObjectTexture" || type == "Texture2D";
		}

		int32_t GraphTextureSlotIndex(const MaterialAsset& material, const Node& textureNode) {
			int32_t slot = 0;
			for(const Node& node : material.graph.nodes) {
				if(!IsGraphTextureNodeType(node.type)) continue;
				if(node.id == textureNode.id) return slot;
				++slot;
			}
			return -1;
		}

		Guid ResolveGraphTextureGuid(const MaterialAsset& material, const Node& textureNode) {
			if(textureNode.type == "Texture2D") return GetGuidProperty(textureNode, "textureGuid");
			if(textureNode.type == "ObjectTexture" && material.objectTextureGuid.isValid()) return material.objectTextureGuid;
			return Guid::Empty();
		}

		Guid FindGraphTextureGuid(const MaterialAsset& material) {
			for(const Node& node : material.graph.nodes) {
				if(!IsGraphTextureNodeType(node.type)) continue;
				const Guid guid = ResolveGraphTextureGuid(material, node);
				if(guid.isValid()) return guid;
			}
			if(material.objectTextureGuid.isValid()) return material.objectTextureGuid;
			return Guid::Empty();
		}

		Guid ResolveTextureInputGuid(const MaterialAsset& material, const Node& node, const char* inputName) {
			const Node* linked = FindLinkedNode(material, node, inputName);
			if(!linked) return Guid::Empty();

			const Guid guid = GetGuidProperty(*linked, "textureGuid");
			if(guid.isValid()) return guid;

			if(IsGraphTextureNodeType(linked->type)) return ResolveGraphTextureGuid(material, *linked);
			return Guid::Empty();
		}

		float PreviewHash(float x, float y) {
			const float n = std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f;
			return n - std::floor(n);
		}

		float PreviewNoise(float x, float y) {
			const float ix = std::floor(x);
			const float iy = std::floor(y);
			const float fx = x - ix;
			const float fy = y - iy;
			const float ux = fx * fx * (3.0f - 2.0f * fx);
			const float uy = fy * fy * (3.0f - 2.0f * fy);

			const float a = PreviewHash(ix, iy);
			const float b = PreviewHash(ix + 1.0f, iy);
			const float c = PreviewHash(ix, iy + 1.0f);
			const float d = PreviewHash(ix + 1.0f, iy + 1.0f);
			const float x0 = a + (b - a) * ux;
			const float x1 = c + (d - c) * ux;
			return x0 + (x1 - x0) * uy;
		}

		void DrawNoiseThumbnail(float scale, ImVec2 size) {
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const ImVec2 max{min.x + size.x, min.y + size.y};
			ImGui::InvisibleButton("##noise-preview", size);

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			constexpr int32_t grid = 36;
			const float cellW = size.x / static_cast<float>(grid);
			const float cellH = size.y / static_cast<float>(grid);
			for(int32_t y = 0; y < grid; ++y) {
				for(int32_t x = 0; x < grid; ++x) {
					const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(grid);
					const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(grid);
					const float value = std::clamp(PreviewNoise(u * scale, v * scale), 0.0f, 1.0f);
					const int32_t c = static_cast<int32_t>(value * 255.0f);
					drawList->AddRectFilled(
						{min.x + x * cellW, min.y + y * cellH},
						{min.x + (x + 1) * cellW + 0.5f, min.y + (y + 1) * cellH + 0.5f},
						IM_COL32(c, c, c, 255));
				}
			}
			drawList->AddRect(min, max, IM_COL32(255, 255, 255, 64), 3.0f);
		}

		bool TryReadTextureDragPayload(Guid& outGuid) {
			const ImGuiPayload* payload = ImGui::GetDragDropPayload();
			if(!payload || !payload->IsDataType("CALYX_ASSET") || payload->DataSize != sizeof(AssetDragPayload)) return false;

			const AssetDragPayload assetPayload = *reinterpret_cast<const AssetDragPayload*>(payload->Data);
			if(assetPayload.type != AssetType::Texture) return false;

			outGuid = assetPayload.guid;
			return outGuid.isValid();
		}

		bool AcceptTextureDropRect(Guid& guid, const ImVec2& min, const ImVec2& max) {
			Guid droppedGuid = Guid::Empty();
			const bool hasTexturePayload = TryReadTextureDragPayload(droppedGuid);
			const bool hovered = hasTexturePayload && ImGui::IsMouseHoveringRect(min, max, true);
			if(!hovered) return false;

			ImGui::GetWindowDrawList()->AddRect(min, max, IM_COL32(120, 180, 255, 235), 4.0f, 0, 3.0f);
			if(!ImGui::IsMouseReleased(ImGuiMouseButton_Left)) return false;

			guid = droppedGuid;
			return true;
		}

		bool DrawTexturePreviewSlot(Guid& guid, ImVec2 size, bool allowDrop = true) {
			bool changed = false;
			const ImVec2 min = ImGui::GetCursorScreenPos();
			const ImVec2 max{min.x + size.x, min.y + size.y};

			if(const AssetRecord* rec = FindTextureRecord(guid); rec && rec->previewTex) {
				ImGui::Image(rec->previewTex, size);
			} else {
				ImGui::InvisibleButton("##texture-preview", size);
			}

			ImDrawList* drawList = ImGui::GetWindowDrawList();
			if(!FindTextureRecord(guid)) {
				drawList->AddRectFilled(min, max, IM_COL32(38, 38, 42, 255), 4.0f);
				drawList->AddText(
					{min.x + 8.0f, min.y + size.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f},
					IM_COL32(180, 180, 185, 255),
					allowDrop ? "Drop Texture" : "No Texture");
			}
			drawList->AddRect(min, max, IM_COL32(255, 255, 255, 55), 4.0f);
			if(allowDrop) changed |= AcceptTextureDropRect(guid, min, max);
			return changed;
		}
	} // namespace

	MaterialNodeEditorPanel::MaterialNodeEditorPanel()
		: IEngineUI("Material Graph"), canvas_("MaterialGraphCanvas") {
		// Shader Graph固有の外観を汎用CanvasへPolicyとして注入する。
		NodeEditorCanvas::Policy policy;
		policy.nodeHeaderColor = [](const Node& node) {
			const NodeTypeDefinition* definition = ShaderGraphNodeRegistry::Get().Find(node.type);
			if(!definition) return ImVec4(0.20f, 0.24f, 0.30f, 1.0f);
			const auto& color = definition->headerColor;
			return ImVec4(color[0], color[1], color[2], color[3]);
		};
		policy.pinColor = [](std::string_view type) {
			if(type == ShaderGraphPinTypes::Material) return ImVec4(0.84f, 0.46f, 1.0f, 1.0f);
			if(type == ShaderGraphPinTypes::Texture2D) return ImVec4(0.22f, 0.76f, 0.82f, 1.0f);
			return ImVec4(0.82f, 0.86f, 0.90f, 1.0f);
		};
		canvas_.SetPolicy(std::move(policy));
		isShow_ = false;
	}

	void MaterialNodeEditorPanel::Render() {
		if(!IsShow()) return;
		bool open = true;
		if(ImGui::Begin(panelName_.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
			if(!selectedMaterial_.isValid()) {
				for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
					if(rec && rec->type == AssetType::Material) {
						selectedMaterial_ = rec->guid;
						break;
					}
				}
			}

			const float sidebarWidth = 260.0f;
			ImGui::BeginChild("##material-sidebar", ImVec2(sidebarWidth, 0.0f), true);
			DrawMaterialList();
			ImGui::EndChild();
			ImGui::SameLine();

			ImGui::BeginChild("##material-workspace", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
			auto material = AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<MaterialAsset>(selectedMaterial_);
			if(material) {
				if(EnsureTextureSampleNodePins(*material)) {
					Evaluate(*material);
					Save(*material);
				}
				EnsureOutputNode(*material);
				DrawToolbar(*material);
				bool pendingGraphCommand = false;
				std::string pendingGraphCommandName;
				NodeGraph pendingGraphBefore;
				NodeGraph pendingGraphAfter;
				if(canvas_.Draw(
					   material->graph,
					   [this, material, &pendingGraphCommand, &pendingGraphCommandName, &pendingGraphBefore, &pendingGraphAfter](Node& node) {
						   const NodeGraph before = material->graph;
						   const bool changed = DrawNodeBody(*material, node);
						   if(changed) {
							   if(ImGui::IsAnyItemActive()) {
								   if(!nodeEditCommandActive_) {
									   nodeEditCommandActive_ = true;
									   nodeEditMaterial_ = material->GetGuid();
									   nodeEditBefore_ = before;
								   }
							   } else {
								   pendingGraphCommand = true;
								   pendingGraphCommandName = "Edit Material Node";
								   pendingGraphBefore = before;
								   pendingGraphAfter = material->graph;
							   }
						   }
						   return changed;
					   },
					   [this, material](const NodeEditorCanvas::ContextMenu& menu) { return DrawContextMenu(*material, menu); },
					   [&pendingGraphCommand, &pendingGraphCommandName, &pendingGraphBefore, &pendingGraphAfter](const char* name, const NodeGraph& before, const NodeGraph& after) {
						   pendingGraphCommand = true;
						   pendingGraphCommandName = name;
						   pendingGraphBefore = before;
						   pendingGraphAfter = after;
					   })) {
					Evaluate(*material);
				}
				if(nodeEditCommandActive_ && !ImGui::IsAnyItemActive()) {
					if(nodeEditMaterial_ == material->GetGuid()) {
						ExecuteGraphCommand(*material, "Edit Material Node", nodeEditBefore_, material->graph);
					}
					nodeEditCommandActive_ = false;
				}
				if(pendingGraphCommand) {
					ExecuteGraphCommand(*material, pendingGraphCommandName.c_str(), pendingGraphBefore, pendingGraphAfter);
				}
				if(DrawLightingModePopup(*material)) {
					Evaluate(*material);
				}
			} else {
				ImGui::Dummy(ImVec2(0.0f, 36.0f));
				ImGui::TextDisabled("No material selected.");
				if(ImGui::Button("Create Material", ImVec2(150.0f, 28.0f))) {
					CreateMaterialAsset();
				}
			}
			ImGui::EndChild();
		}
		ImGui::End();
		if(!open) SetShow(false);
	}

	void MaterialNodeEditorPanel::DrawMaterialList() {
		ImGui::TextUnformatted("Materials");
		ImGui::SameLine();
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 78.0f);
		if(ImGui::Button("New", ImVec2(58.0f, 24.0f))) {
			CreateMaterialAsset();
		}
		ImGui::Separator();

		ImGui::BeginChild("##material-list", ImVec2(0.0f, -32.0f), false);
		for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
			if(!rec || rec->type != AssetType::Material) continue;
			bool selected = rec->guid == selectedMaterial_;
			ImGui::PushID(rec->guid.ToString().c_str());
			const std::string label = rec->sourcePath.stem().string();
			if(ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0.0f, 28.0f))) {
				selectedMaterial_ = rec->guid;
				if(ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					BeginRenameMaterial(rec->guid, rec->sourcePath);
				}
			}
			if(ImGui::BeginPopupContextItem("MaterialContext")) {
				if(ImGui::MenuItem("Rename")) BeginRenameMaterial(rec->guid, rec->sourcePath);
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}
		ImGui::EndChild();

		if(renamePopupRequested_) {
			ImGui::OpenPopup("Rename Material");
			renamePopupRequested_ = false;
		}
		if(ImGui::BeginPopupModal("Rename Material", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			if(focusRename_) {
				ImGui::SetKeyboardFocusHere();
				focusRename_ = false;
			}
			ImGui::SetNextItemWidth(260.0f);
			const bool submitted = ImGui::InputText("Name", renameBuffer_.data(), renameBuffer_.size(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
			if(submitted) CommitRenameMaterial();
			if(ImGui::Button("Rename", ImVec2(96.0f, 0.0f))) CommitRenameMaterial();
			ImGui::SameLine();
			if(ImGui::Button("Cancel", ImVec2(96.0f, 0.0f)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				CancelRenameMaterial();
			}
			ImGui::EndPopup();
		}
	}

	void MaterialNodeEditorPanel::DrawMaterialPreview(MaterialAsset& material, bool framed) {
		if(framed) {
			ImGui::BeginChild("##material-preview", ImVec2(0.0f, 188.0f), true);
		}
		ImGui::TextUnformatted("Preview");
		ImGui::SameLine();
		ImGui::TextDisabled("Base Color");

		const MaterialGraphRuntimeShader shader = runtimeShaderCache_.GetOrCompilePreviewPixelShader(material);
		if(!shader.pixelShader || !shader.compileSucceeded) {
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "Preview shader compile failed.");
			if(!shader.compileMessage.empty()) {
				ImGui::TextWrapped("%s", shader.compileMessage.c_str());
			}
			if(framed) ImGui::EndChild();
			return;
		}

		if(!EnsurePreviewResources() || !EnsurePreviewPipeline(shader)) {
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "Preview renderer is not available.");
			if(framed) ImGui::EndChild();
			return;
		}

		ID3D12GraphicsCommandList* cmd = GraphicsGroup::GetInstance()->GetCommandList().Get();
		if(cmd && previewTarget_) {
			if(ID3D12DescriptorHeap* heap = DescriptorAllocator::GetHeap(DescriptorUsage::CbvSrvUav)) {
				cmd->SetDescriptorHeaps(1, &heap);
			}

			previewMaterialBuffer_.TransferData(BuildPreviewMaterial(material));
			previewTarget_->Clear(cmd);
			previewTarget_->SetRenderTarget(cmd);

			const PipelineSet set = previewPipeline_->GetPipelineSet();
			set.SetCommand(cmd);
			previewMaterialBuffer_.SetCommand(GraphicsGroup::GetInstance()->GetCommandList(), 0);
			cmd->SetGraphicsRootDescriptorTable(1, ResolvePreviewTexture(material));
			cmd->SetGraphicsRootDescriptorTable(2, ResolvePreviewTextureTable(material));
			cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmd->DrawInstanced(3, 1, 0, 0);

			previewTarget_->TransitionTo(cmd, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}

		const ImTextureID previewTex = reinterpret_cast<ImTextureID>(previewTarget_->GetSRV().ptr);
		const float imageSize = framed ? 144.0f : 172.0f;
		ImGui::Image(previewTex, ImVec2(imageSize, imageSize));
		if(framed) ImGui::SameLine();
		ImGui::BeginGroup();
		if(framed) {
			ImGui::TextDisabled("Shader: %zu", shader.hash);
		} else {
			ImGui::TextDisabled("Shader: %08zx", shader.hash & 0xffffffffu);
		}
		ImGui::TextDisabled("%s", shader.cacheHit ? "Cache hit" : "Compiled");
		if(shader.usesObjectTexture) {
			ImGui::TextDisabled("Uses Object Texture");
		}
		ImGui::EndGroup();
		if(framed) ImGui::EndChild();
	}

	void MaterialNodeEditorPanel::DrawToolbar(MaterialAsset& material) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 6.0f));
		ImGui::TextUnformatted(material.GetName().c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("GUID: %s", material.GetGuid().ToString().substr(0, 8).c_str());
		ImGui::SameLine();
		if(ImGui::Button("+ Add Node", ImVec2(112.0f, 0.0f))) ImGui::OpenPopup("AddNodeToolbar");
		if(ImGui::BeginPopup("AddNodeToolbar")) {
			Vector2 position = canvas_.GetLastViewCenter();
			position.x -= 110.0f;
			position.y -= 60.0f;
			DrawAddNodeMenu(material, position);
			ImGui::EndPopup();
		}
		ImGui::SameLine();
		if(ImGui::Button("Save", ImVec2(76.0f, 0.0f))) Save(material);
		ImGui::SameLine();
		if(ImGui::Button("Copy HLSL", ImVec2(96.0f, 0.0f))) {
			const GeneratedShaderGraphCode generated = ShaderGraphCodeGenerator::GenerateObject3DMaterialFunction(material);
			ImGui::SetClipboardText(generated.hlsl.c_str());
			graphStatusMessage_ = "Copied generated material HLSL to clipboard.";
			graphStatusIsError_ = false;
		}
		ImGui::SameLine();
		if(ImGui::Button("Compile HLSL", ImVec2(112.0f, 0.0f))) {
			MaterialGraphRuntimeShader shader = runtimeShaderCache_.GetOrCompilePreviewPixelShader(material);
			if(shader.compileSucceeded) {
				std::ostringstream oss;
				oss << "Generated preview pixel shader " << (shader.cacheHit ? "cache hit" : "compiled")
					<< ". Hash: " << shader.hash << ".";
				if(!shader.compileMessage.empty()) oss << "\n" << shader.compileMessage;
				graphStatusMessage_ = oss.str();
				graphStatusIsError_ = false;
			} else {
				std::ostringstream oss;
				oss << "Generated preview pixel shader compile failed.";
				if(shader.fallbackUsed) oss << " Using last successful preview shader.";
				if(!shader.compileMessage.empty()) oss << "\n" << shader.compileMessage;
				graphStatusMessage_ = oss.str();
				graphStatusIsError_ = true;
			}
		}
		ImGui::SameLine();
		if(ImGui::Button("Build Runtime", ImVec2(116.0f, 0.0f))) {
			MaterialGraphRuntimeShader shader = runtimeShaderCache_.GetOrCompileObject3DPixelShader(material);
			std::ostringstream oss;
			if(shader.compileSucceeded) {
				oss << "Runtime material shader " << (shader.cacheHit ? "cache hit" : "compiled")
					<< ". Hash: " << shader.hash
					<< ", cache entries: " << runtimeShaderCache_.Size() << ".";
				if(!shader.compileMessage.empty()) oss << "\n" << shader.compileMessage;
			} else {
				oss << "Runtime material shader compile failed.";
				if(shader.fallbackUsed) oss << " Using last successful runtime shader.";
				else oss << " No fallback shader is available.";
				if(!shader.compileMessage.empty()) oss << "\n" << shader.compileMessage;
			}
			graphStatusMessage_ = oss.str();
			graphStatusIsError_ = !shader.compileSucceeded;
		}
		ImGui::SameLine();
		if(ImGui::Button("Validate Graph", ImVec2(124.0f, 0.0f))) {
			const ShaderGraphValidationResult validation = ShaderGraphValidator::ValidateMaterialGraph(material.graph);
			std::ostringstream oss;
			oss << (validation.ok ? "Material graph validation passed." : "Material graph validation failed:");
			for(const std::string& message : validation.messages) {
				oss << "\n- " << message;
			}
			graphStatusMessage_ = oss.str();
			graphStatusIsError_ = !validation.ok;
		}
		ImGui::SameLine();
		if(ImGui::Button("Validate Shader", ImVec2(124.0f, 0.0f))) {
			ShaderCompiler compiler;
			compiler.InitializeDXC();
			const ShaderReflectionInfo reflection = compiler.ReflectShader(L"Resources/shaders/Core/Object3d.PS.hlsl", L"ps_6_5");
			const ShaderGraphValidationResult validation = ShaderGraphValidator::ValidateObject3DMaterialShader(reflection);
			if(validation.ok) {
				std::ostringstream oss;
				oss << "Object3D shader contract is valid. Resources: " << reflection.resources.size()
					<< ", cbuffers: " << reflection.cbuffers.size() << ".";
				graphStatusMessage_ = oss.str();
				graphStatusIsError_ = false;
			} else {
				std::ostringstream oss;
				oss << "Object3D shader contract validation failed:";
				for(const std::string& message : validation.messages) {
					oss << "\n- " << message;
				}
				graphStatusMessage_ = oss.str();
				graphStatusIsError_ = true;
			}
		}
		ImGui::PopStyleVar();
		ImGui::Separator();
		if(!graphStatusMessage_.empty()) {
			const ImVec4 color = graphStatusIsError_ ? ImVec4(1.0f, 0.35f, 0.25f, 1.0f) : ImVec4(0.45f, 0.85f, 0.55f, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextWrapped("%s", graphStatusMessage_.c_str());
			ImGui::PopStyleColor();
			ImGui::Separator();
		}
	}

	bool MaterialNodeEditorPanel::DrawAddNodeMenu(MaterialAsset& material, Vector2 position) {
		const NodeGraph before = material.graph;
		bool changed = false;
		const ShaderGraphSchema& registry = ShaderGraphNodeRegistry::Get();
		constexpr std::string_view categories[] = {
			"Material Parameters", "Lighting", "Master", "Textures", "Inputs", "Operators"};

		// RegistryのCategory単位で追加メニューを自動構築する。
		for(const std::string_view category : categories) {
			if(!ImGui::BeginMenu(category.data())) continue;
			for(const NodeTypeDefinition& definition : registry.Definitions()) {
				if(definition.category != category || definition.type == "Output") continue;
				if(ImGui::MenuItem(definition.title.c_str())) {
					changed |= AddRegisteredNode(material, definition.type, position);
				}
			}
			ImGui::EndMenu();
		}

		// 変更内容を一つのUndo Commandとして登録する。
		if(changed) ExecuteGraphCommand(material, "Create Material Node", before, material.graph);
		return changed;
	}

	void MaterialNodeEditorPanel::ExecuteGraphCommand(MaterialAsset& material, const char* name, const NodeGraph& before, const NodeGraph& after) {
		auto apply = [this, materialGuid = material.GetGuid()](const NodeGraph& graph) {
			auto material = AssetManager::GetInstance()->GetDataAssetManager()->GetAsset<MaterialAsset>(materialGuid);
			if(!material) return;
			material->graph = graph;
			material->graph.EnsureNextId();
			Evaluate(*material);
		};
		CommandManager::GetInstance()->Execute(
			std::make_unique<ValueEditCommand<NodeGraph>>(name, before, after, apply));
		EngineLogger::GetInstance().Add(
			LogLevel::Info,
			LogCategory::Asset,
			"Material graph changed: " + material.GetName() + " (" + name + ")",
			"MaterialNodeEditorPanel");
	}

	bool MaterialNodeEditorPanel::DrawContextMenu(MaterialAsset& material, const NodeEditorCanvas::ContextMenu& menu) {
		bool changed = false;
		if(menu.type == NodeEditorCanvas::ContextMenuType::Background) {
			changed |= DrawAddNodeMenu(material, menu.canvasPosition);
		} else {
			ImGui::TextDisabled("Delete: select node and press Delete");
		}
		return changed;
	}

	bool MaterialNodeEditorPanel::DrawLightingModePopup(MaterialAsset& material) {
		bool changed = false;
		if(lightingModePopupRequested_) {
			ImGui::OpenPopup("LightingModeSelect");
			lightingModePopupRequested_ = false;
		}

		ImGui::SetNextWindowPos(ImVec2(lightingModePopupPos_.x, lightingModePopupPos_.y), ImGuiCond_Appearing);
		ImGui::SetNextWindowSizeConstraints(ImVec2(180.0f, 0.0f), ImVec2(240.0f, 260.0f));
		if(ImGui::BeginPopup("LightingModeSelect")) {
			Node* target = nullptr;
			for(auto& node : material.graph.nodes) {
				if(node.id == lightingModePopupNodeId_) {
					target = &node;
					break;
				}
			}

			if(target && target->type == "LightingMode") {
				for(int32_t i = 0; i < kLightingModeCount; ++i) {
					const bool selected = target->GetProperty<int32_t>("value", 0) == i;
					if(ImGui::Selectable(kLightingModes[i], selected)) {
						target->SetProperty("value", i);
						material.lightingMode = i;
						changed				  = true;
						ImGui::CloseCurrentPopup();
					}
					if(selected) ImGui::SetItemDefaultFocus();
				}
			} else {
				ImGui::TextDisabled("No lighting mode node.");
			}

			if(ImGui::IsKeyPressed(ImGuiKey_Escape)) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		return changed;
	}

	void MaterialNodeEditorPanel::CreateMaterialAsset() {
		auto*				  db	 = AssetDatabase::GetInstance();
		std::filesystem::path folder = db->GetRoot() / "Materials";
		std::filesystem::create_directories(folder);

		std::filesystem::path path = folder / "New Material.mat";
		for(int i = 1; std::filesystem::exists(path); ++i) {
			path = folder / ("New Material " + std::to_string(i) + ".mat");
		}

		{
			std::ofstream ofs(path);
			if(!ofs) return;
		}

		const Guid guid = db->RegisterOrUpdate(path, AssetType::Material);
		if(auto* dataAssets = AssetManager::GetInstance()->GetDataAssetManager()) {
			auto asset = dataAssets->GetAsset<MaterialAsset>(guid);
			if(asset) {
				asset->SetName(path.stem().string());
				dataAssets->SaveAsset(*asset, path);
			}
		}
		db->Scan();
		selectedMaterial_ = guid;
	}

	void MaterialNodeEditorPanel::BeginRenameMaterial(const Guid& guid, const std::filesystem::path& path) {
		renamingMaterial_	   = guid;
		renamingPath_		   = path;
		const std::string stem = path.stem().string();
		std::fill(renameBuffer_.begin(), renameBuffer_.end(), '\0');
		std::copy_n(stem.c_str(), std::min(stem.size(), renameBuffer_.size() - 1), renameBuffer_.data());
		focusRename_		  = true;
		renamePopupRequested_ = true;
	}

	void MaterialNodeEditorPanel::CommitRenameMaterial() {
		std::string newName = renameBuffer_.data();
		auto		first	= newName.find_first_not_of(" \t\r\n");
		auto		last	= newName.find_last_not_of(" \t\r\n");
		if(first == std::string::npos) return;
		newName = newName.substr(first, last - first + 1);
		if(newName.empty() || renamingPath_.empty()) return;

		std::filesystem::path newPath = renamingPath_.parent_path() / (newName + renamingPath_.extension().string());
		if(newPath != renamingPath_ && std::filesystem::exists(newPath)) return;

		std::error_code ec;
		if(newPath != renamingPath_) {
			std::filesystem::rename(renamingPath_, newPath, ec);
			if(ec) return;
			std::filesystem::path oldMeta = renamingPath_;
			oldMeta += ".meta";
			std::filesystem::path newMeta = newPath;
			newMeta += ".meta";
			if(std::filesystem::exists(oldMeta)) {
				std::filesystem::rename(oldMeta, newMeta, ec);
			}
		}

		if(auto* dataAssets = AssetManager::GetInstance()->GetDataAssetManager()) {
			if(auto asset = dataAssets->GetAsset<MaterialAsset>(renamingMaterial_)) {
				asset->SetName(newName);
				dataAssets->SaveAsset(*asset, newPath);
			}
		}
		AssetDatabase::GetInstance()->RegisterOrUpdate(newPath, AssetType::Material);
		AssetDatabase::GetInstance()->Scan();
		selectedMaterial_ = renamingMaterial_;
		CancelRenameMaterial();
	}

	void MaterialNodeEditorPanel::CancelRenameMaterial() {
		renamingMaterial_ = Guid::Empty();
		renamingPath_.clear();
		focusRename_		  = false;
		renamePopupRequested_ = false;
		ImGui::CloseCurrentPopup();
	}

	bool MaterialNodeEditorPanel::DrawNodeBody(MaterialAsset& material, Node& node) {
		bool changed = false;
		ImGui::PushID(node.id);
		if(node.type == "Color") {
			Vector4 value = GetColorProperty(node, "value", {1, 1, 1, 1});
			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::ColorEdit4("Color", &value.x)) {
				SetColorProperty(node, "value", value);
				changed = true;
			}
		} else if(node.type == "Float2") {
			float x = GetFloatProperty(node, "x", 0.0f);
			float y = GetFloatProperty(node, "y", 0.0f);
			ImGui::SetNextItemWidth(86.0f);
			if(ImGui::DragFloat("X", &x, 0.01f)) {
				SetFloatProperty(node, "x", x);
				changed = true;
			}
			ImGui::SameLine();
			ImGui::SetNextItemWidth(86.0f);
			if(ImGui::DragFloat("Y", &y, 0.01f)) {
				SetFloatProperty(node, "y", y);
				changed = true;
			}
		} else if(node.type == "Float" || node.type == "Shininess" || node.type == "Roughness") {
			ImGui::SetNextItemWidth(178.0f);
			const float minValue = node.type == "Float" ? -1000.0f : 0.0f;
			const float maxValue = node.type == "Float" ? 1000.0f : 256.0f;
			float value = node.GetProperty<float>("value", 0.0f);
			if(ImGui::DragFloat("Value", &value, 0.01f, minValue, maxValue)) {
				node.SetProperty("value", value);
				changed = true;
			}
		} else if(node.type == "Reflect") {
			bool value = node.GetProperty<bool>("value", false);
			if(ImGui::Checkbox("Value", &value)) {
				node.SetProperty("value", value);
				changed = true;
			}
		} else if(node.type == "LightingMode") {
			const int32_t value = std::clamp(node.GetProperty<int32_t>("value", 0), 0, kLightingModeCount - 1);
			node.SetProperty("value", value);
			const char* current = kLightingModes[value];
			ImGui::SetNextItemWidth(178.0f);
			if(ImGui::Button(current, ImVec2(178.0f, 0.0f))) {
				lightingModePopupRequested_ = true;
				lightingModePopupNodeId_	= node.id;
				const ImVec2 itemMin		= ImGui::GetItemRectMin();
				const ImVec2 itemMax		= ImGui::GetItemRectMax();
				ImVec2		 popupPos(itemMax.x + 8.0f, itemMin.y);
				if(ImGuiViewport* viewport = ImGui::GetWindowViewport()) {
					const ImVec2 workMin		 = viewport->WorkPos;
					const ImVec2 workMax		 = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y);
					const float	 estimatedWidth	 = 220.0f;
					const float	 estimatedHeight = 180.0f;
					if(popupPos.x + estimatedWidth > workMax.x) {
						popupPos.x = itemMin.x - estimatedWidth - 8.0f;
					}
					if(popupPos.x < workMin.x) {
						popupPos.x = workMin.x;
					}
					if(popupPos.y + estimatedHeight > workMax.y) {
						popupPos.y = workMax.y - estimatedHeight;
						if(popupPos.y < workMin.y) {
							popupPos.y = workMin.y;
						}
					}
				}
				lightingModePopupPos_.x = popupPos.x;
				lightingModePopupPos_.y = popupPos.y;
			}
		} else if(node.type == "ToonMaster") {
			if(ImGui::Button("Reset Toon Master", ImVec2(188.0f, 0.0f))) {
				SetDefaultToonMasterProperties(node);
				changed = true;
			}

			Vector4 base = GetColorProperty(node, "baseColor", {1, 1, 1, 1});
			Vector4 emissive = GetColorProperty(node, "emissiveColor", material.emissiveColor);
			Vector4 highlight = GetColorProperty(node, "highlightColor", {1.08f, 1.06f, 1.02f, 1.0f});
			Vector4 firstShade = GetColorProperty(node, "firstShadeColor", {0.72f, 0.76f, 0.86f, 1.0f});
			Vector4 secondShade = GetColorProperty(node, "secondShadeColor", {0.42f, 0.46f, 0.58f, 1.0f});
			float baseStep = GetFloatProperty(node, "baseStep", 0.25f);
			float baseFeather = GetFloatProperty(node, "baseFeather", 0.03f);
			float shadeStep = GetFloatProperty(node, "shadeStep", -0.15f);
			float shadeFeather = GetFloatProperty(node, "shadeFeather", 0.03f);
			float specThreshold = GetFloatProperty(node, "specularThreshold", 0.96f);
			float specSoftness = GetFloatProperty(node, "specularSoftness", 0.02f);
			float specIntensity = GetFloatProperty(node, "specularIntensity", 0.35f);
			float emissiveIntensity = GetFloatProperty(node, "emissiveIntensity", material.emissiveIntensity);

			auto drawColorFallback = [&](const char* pinName, const char* label, const char* property, Vector4& value) {
				const bool linked = IsInputLinked(material, node, pinName);
				ImGui::BeginDisabled(linked);
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4(label, &value.x)) {
					SetColorProperty(node, property, value);
					changed = true;
				}
				ImGui::EndDisabled();
				if(linked) {
					ImGui::SameLine();
					ImGui::TextDisabled("Linked");
				}
			};
			auto drawFloatFallback = [&](const char* pinName, const char* label, const char* property, float& value, float minValue, float maxValue) {
				const bool linked = IsInputLinked(material, node, pinName);
				ImGui::BeginDisabled(linked);
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat(label, &value, minValue, maxValue)) {
					SetFloatProperty(node, property, value);
					changed = true;
				}
				ImGui::EndDisabled();
				if(linked) {
					ImGui::SameLine();
					ImGui::TextDisabled("Linked");
				}
			};

			drawColorFallback("Base Color", "Base", "baseColor", base);
			drawColorFallback("Emissive", "Emissive", "emissiveColor", emissive);
			drawFloatFallback("Emissive Intensity", "Emission", "emissiveIntensity", emissiveIntensity, 0.0f, 20.0f);
			drawColorFallback("Highlight", "Highlight", "highlightColor", highlight);
			drawColorFallback("1st Shade", "1st Shade", "firstShadeColor", firstShade);
			drawColorFallback("2nd Shade", "2nd Shade", "secondShadeColor", secondShade);
			drawFloatFallback("Base Step", "Base Step", "baseStep", baseStep, -1.0f, 1.0f);
			drawFloatFallback("Base Feather", "Base Feather", "baseFeather", baseFeather, 0.0f, 0.25f);
			drawFloatFallback("Shade Step", "Shade Step", "shadeStep", shadeStep, -1.0f, 1.0f);
			drawFloatFallback("Shade Feather", "Shade Feather", "shadeFeather", shadeFeather, 0.0f, 0.25f);
			drawFloatFallback("Spec Threshold", "Spec Threshold", "specularThreshold", specThreshold, 0.0f, 1.0f);
			drawFloatFallback("Spec Softness", "Spec Softness", "specularSoftness", specSoftness, 0.0f, 0.25f);
			drawFloatFallback("Spec Intensity", "Spec Intensity", "specularIntensity", specIntensity, 0.0f, 4.0f);
		} else if(node.type == "LitMaster") {
			ImGui::TextDisabled("Standard Lit Surface");
			Vector4 emissive = GetColorProperty(node, "emissiveColor", material.emissiveColor);
			float emissiveIntensity = GetFloatProperty(node, "emissiveIntensity", material.emissiveIntensity);
			const bool emissiveLinked = IsInputLinked(material, node, "Emissive");
			ImGui::BeginDisabled(emissiveLinked);
			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::ColorEdit4("Emissive", &emissive.x)) {
				SetColorProperty(node, "emissiveColor", emissive);
				changed = true;
			}
			ImGui::EndDisabled();
			const bool intensityLinked = IsInputLinked(material, node, "Emissive Intensity");
			ImGui::BeginDisabled(intensityLinked);
			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::SliderFloat("Emission", &emissiveIntensity, 0.0f, 20.0f)) {
				SetFloatProperty(node, "emissiveIntensity", emissiveIntensity);
				changed = true;
			}
			ImGui::EndDisabled();
		} else if(node.type == "UnlitMaster") {
			ImGui::TextDisabled("Unlit Surface");
			Vector4 emissive = GetColorProperty(node, "emissiveColor", material.emissiveColor);
			float emissiveIntensity = GetFloatProperty(node, "emissiveIntensity", material.emissiveIntensity);
			const bool emissiveLinked = IsInputLinked(material, node, "Emissive");
			ImGui::BeginDisabled(emissiveLinked);
			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::ColorEdit4("Emissive", &emissive.x)) {
				SetColorProperty(node, "emissiveColor", emissive);
				changed = true;
			}
			ImGui::EndDisabled();
			const bool intensityLinked = IsInputLinked(material, node, "Emissive Intensity");
			ImGui::BeginDisabled(intensityLinked);
			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::SliderFloat("Emission", &emissiveIntensity, 0.0f, 20.0f)) {
				SetFloatProperty(node, "emissiveIntensity", emissiveIntensity);
				changed = true;
			}
			ImGui::EndDisabled();
		} else if(IsLightingNode(node.type)) {
			if(node.type == "ToonLighting") {
				if(ImGui::Button("Reset Toon Defaults", ImVec2(188.0f, 0.0f))) {
					SetDefaultToonProperties(node);
					changed = true;
				}

				Vector4 highlight = GetColorProperty(node, "toonHighlightColor", {1.15f, 1.10f, 1.00f, 1.0f});
				Vector4 base = GetColorProperty(node, "toonBaseColor", {1.0f, 1.0f, 1.0f, 1.0f});
				Vector4 midShadow = GetColorProperty(node, "toonMidShadowColor", {0.72f, 0.76f, 0.86f, 1.0f});
				Vector4 shadow = GetColorProperty(node, "toonShadowColor", {0.42f, 0.46f, 0.58f, 1.0f});
				float threshold1 = GetFloatProperty(node, "toonThreshold1", -0.15f);
				float threshold2 = GetFloatProperty(node, "toonThreshold2", 0.25f);
				float threshold3 = GetFloatProperty(node, "toonThreshold3", 0.82f);
				float edgeSoftness = GetFloatProperty(node, "toonEdgeSoftness", 0.03f);
				float specThreshold = GetFloatProperty(node, "toonSpecularThreshold", 0.96f);
				float specSoftness = GetFloatProperty(node, "toonSpecularSoftness", 0.02f);
				float specIntensity = GetFloatProperty(node, "toonSpecularIntensity", 0.35f);

				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Highlight", &highlight.x)) {
					SetColorProperty(node, "toonHighlightColor", highlight);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Base", &base.x)) {
					SetColorProperty(node, "toonBaseColor", base);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Mid Shadow", &midShadow.x)) {
					SetColorProperty(node, "toonMidShadowColor", midShadow);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::ColorEdit4("Shadow", &shadow.x)) {
					SetColorProperty(node, "toonShadowColor", shadow);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Threshold 1", &threshold1, -1.0f, 1.0f)) {
					SetFloatProperty(node, "toonThreshold1", threshold1);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Threshold 2", &threshold2, -1.0f, 1.0f)) {
					SetFloatProperty(node, "toonThreshold2", threshold2);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Threshold 3", &threshold3, -1.0f, 1.0f)) {
					SetFloatProperty(node, "toonThreshold3", threshold3);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Edge Softness", &edgeSoftness, 0.0f, 0.25f)) {
					SetFloatProperty(node, "toonEdgeSoftness", edgeSoftness);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Spec Threshold", &specThreshold, 0.0f, 1.0f)) {
					SetFloatProperty(node, "toonSpecularThreshold", specThreshold);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Spec Softness", &specSoftness, 0.0f, 0.25f)) {
					SetFloatProperty(node, "toonSpecularSoftness", specSoftness);
					changed = true;
				}
				ImGui::SetNextItemWidth(188.0f);
				if(ImGui::SliderFloat("Spec Intensity", &specIntensity, 0.0f, 4.0f)) {
					SetFloatProperty(node, "toonSpecularIntensity", specIntensity);
					changed = true;
				}
			} else {
				ImGui::TextDisabled("%s", kLightingModes[LightingModeFromNodeType(node.type, 0)]);
			}
		} else if(node.type == "AddFloat" || node.type == "AddFloat2") {
			ImGui::TextDisabled("A + B");
		} else if(node.type == "SubtractFloat" || node.type == "SubtractFloat2") {
			ImGui::TextDisabled("A - B");
		} else if(node.type == "MultiplyColor" || node.type == "MultiplyFloat" || node.type == "MultiplyFloat2") {
			ImGui::TextDisabled("A * B");
		} else if(node.type == "DivideFloat" || node.type == "DivideFloat2") {
			ImGui::TextDisabled("A / B");
		} else if(node.type == "PowerFloat") {
			ImGui::TextDisabled("pow(A, B)");
		} else if(node.type == "MinFloat") {
			ImGui::TextDisabled("min(A, B)");
		} else if(node.type == "MaxFloat") {
			ImGui::TextDisabled("max(A, B)");
		} else if(node.type == "LerpColor" || node.type == "LerpFloat") {
			ImGui::TextDisabled("lerp(A, B, T)");
		} else if(node.type == "SaturateFloat") {
			ImGui::TextDisabled("saturate(Value)");
		} else if(node.type == "FracFloat") {
			ImGui::TextDisabled("frac(Value)");
		} else if(node.type == "OneMinusFloat") {
			ImGui::TextDisabled("1 - Value");
		} else if(node.type == "StepFloat") {
			ImGui::TextDisabled("Value >= Edge ? 1 : 0");
		} else if(node.type == "SmoothstepFloat") {
			ImGui::TextDisabled("smoothstep(Edge0, Edge1, Value)");
		} else if(node.type == "CombineFloat2") {
			ImGui::TextDisabled("float2(X, Y)");
		} else if(node.type == "SplitFloat2") {
			ImGui::TextDisabled("float2 -> X/Y");
		} else if(node.type == "UVTransform") {
			ImGui::TextDisabled("UV * Scale + Offset");
			float scaleX = GetFloatProperty(node, "scaleX", 1.0f);
			float scaleY = GetFloatProperty(node, "scaleY", 1.0f);
			float offsetX = GetFloatProperty(node, "offsetX", 0.0f);
			float offsetY = GetFloatProperty(node, "offsetY", 0.0f);
			const bool scaleLinked = IsInputLinked(material, node, "Scale");
			const bool offsetLinked = IsInputLinked(material, node, "Offset");
			ImGui::BeginDisabled(scaleLinked);
			ImGui::SetNextItemWidth(86.0f);
			if(ImGui::DragFloat("Scale X", &scaleX, 0.01f)) { SetFloatProperty(node, "scaleX", scaleX); changed = true; }
			ImGui::SameLine();
			ImGui::SetNextItemWidth(86.0f);
			if(ImGui::DragFloat("Scale Y", &scaleY, 0.01f)) { SetFloatProperty(node, "scaleY", scaleY); changed = true; }
			ImGui::EndDisabled();
			ImGui::BeginDisabled(offsetLinked);
			ImGui::SetNextItemWidth(86.0f);
			if(ImGui::DragFloat("Offset X", &offsetX, 0.01f)) { SetFloatProperty(node, "offsetX", offsetX); changed = true; }
			ImGui::SameLine();
			ImGui::SetNextItemWidth(86.0f);
			if(ImGui::DragFloat("Offset Y", &offsetY, 0.01f)) { SetFloatProperty(node, "offsetY", offsetY); changed = true; }
			ImGui::EndDisabled();
		} else if(node.type == "ObjectTexture") {
			ImGui::TextDisabled("Object / model texture");
			Guid textureGuid = ResolveGraphTextureGuid(material, node);
			DrawTexturePreviewSlot(textureGuid, ImVec2(112.0f, 112.0f), false);
		} else if(node.type == "Texture2D") {
			ImGui::TextDisabled("Texture2D asset");
			Guid textureGuid = ResolveGraphTextureGuid(material, node);
			if(DrawTextureAssetSelector("Texture", textureGuid)) {
				SetGuidProperty(node, "textureGuid", textureGuid);
				changed = true;
			}
			if(DrawTexturePreviewSlot(textureGuid, ImVec2(112.0f, 112.0f))) {
				SetGuidProperty(node, "textureGuid", textureGuid);
				changed = true;
			}
		} else if(node.type == "TextureSample") {
			ImGui::TextDisabled("Sample Texture2D");
			Guid textureGuid = ResolveTextureInputGuid(material, node, "Texture");
			DrawTexturePreviewSlot(textureGuid, ImVec2(112.0f, 112.0f), false);
		} else if(node.type == "NoiseTexture") {
			float scale = GetFloatProperty(node, "scale", 8.0f);
			const bool linked = IsInputLinked(material, node, "Scale");
			ImGui::BeginDisabled(linked);
			ImGui::SetNextItemWidth(188.0f);
			if(ImGui::SliderFloat("Scale", &scale, 0.1f, 128.0f)) {
				SetFloatProperty(node, "scale", scale);
				changed = true;
			}
			ImGui::EndDisabled();
			if(linked) {
				ImGui::SameLine();
				ImGui::TextDisabled("Linked");
			}
			DrawNoiseThumbnail(scale, ImVec2(112.0f, 112.0f));
		} else if(node.type == "UV" || node.type == "UVX" || node.type == "UVY" || node.type == "Time" ||
				  node.type == "WorldPositionX" || node.type == "WorldPositionY" || node.type == "WorldPositionZ" ||
				  node.type == "WorldNormalX" || node.type == "WorldNormalY" || node.type == "WorldNormalZ" ||
				  node.type == "ViewDirectionX" || node.type == "ViewDirectionY" || node.type == "ViewDirectionZ") {
			ImGui::TextDisabled("Shader input");
		} else if(node.type == "Output") {
			ImGui::TextUnformatted("Material Output");
			DrawMaterialPreview(material, false);
		}
		ImGui::PopID();
		return changed;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		Registry定義からMaterial Nodeを生成
	/////////////////////////////////////////////////////////////////////////////////////////
	bool MaterialNodeEditorPanel::AddRegisteredNode(MaterialAsset& material, std::string_view type, Vector2 position) {
		const ShaderGraphSchema& registry = ShaderGraphNodeRegistry::Get();
		Node node = registry.CreateNode(material.graph, type, position);
		if(node.id == 0) return false;

		// Material依存の初期値だけを生成後に上書きする。
		if(node.type == "Color") SetColorProperty(node, "value", material.color);
		if(node.type == "Shininess") node.SetProperty("value", material.shininess);
		if(node.type == "Roughness") node.SetProperty("value", material.roughness);
		if(node.type == "Reflect") node.SetProperty("value", material.isReflect);
		if(node.type == "LightingMode") node.SetProperty("value", std::clamp(material.lightingMode, 0, kLightingModeCount - 1));
		if(node.type == "HalfLambertLighting") node.SetProperty("value", 0);
		if(node.type == "LambertLighting") node.SetProperty("value", 1);
		if(node.type == "ToonLighting") {
			node.SetProperty("value", 2);
			SetDefaultToonProperties(node);
		}
		if(node.type == "NoLighting") node.SetProperty("value", 3);
		if(node.type == "UnlitColorLighting") node.SetProperty("value", 4);
		if(node.type == "ToonMaster") SetDefaultToonMasterProperties(node);
		if(node.type == "LitMaster") {
			node.SetProperty("lightingMode", 0.0f);
			SetColorProperty(node, "emissiveColor", material.emissiveColor);
			node.SetProperty("emissiveIntensity", material.emissiveIntensity);
			SetFloatProperty(node, "shininess", material.shininess);
			SetFloatProperty(node, "roughness", material.roughness);
		}
		if(node.type == "UnlitMaster") {
			SetColorProperty(node, "emissiveColor", material.emissiveColor);
			node.SetProperty("emissiveIntensity", material.emissiveIntensity);
		}

		material.graph.nodes.push_back(std::move(node));
		return true;
	}


	void MaterialNodeEditorPanel::EnsureOutputNode(MaterialAsset& material) {
		const ShaderGraphSchema& schema = ShaderGraphNodeRegistry::Get();

		for(auto& node : material.graph.nodes) {
			if(const ShaderGraphNodeSchema* schemaNode = schema.Find(node.type)) {
				EnsureSchemaPins(material, node, *schemaNode);
			}
		}

		for(auto& node : material.graph.nodes) {
			if(node.type != "Output") continue;
			auto ensureInput = [&material, &node](const char* name, std::string_view pinType) {
				const auto exists = std::any_of(node.inputs.begin(), node.inputs.end(), [name](const NodePin& pin) {
					return pin.name == name;
				});
				if(!exists) node.inputs.push_back({material.graph.AllocateId(), name, NodePinKind::Input, std::string(pinType)});
			};
			ensureInput("Surface", ShaderGraphPinTypes::Material);

			const auto surfaceIt = std::find_if(node.inputs.begin(), node.inputs.end(), [](const NodePin& pin) {
				return pin.name == "Surface";
			});
			const bool hasSurfaceLink = surfaceIt != node.inputs.end() &&
				std::any_of(material.graph.links.begin(), material.graph.links.end(), [&surfaceIt](const NodeLink& link) {
					return link.toPinId == surfaceIt->id;
				});

			if(hasSurfaceLink) {
				std::vector<int32_t> removedPinIds;
				for(const auto& pin : node.inputs) {
					if(IsLegacyOutputPin(pin.name)) {
						removedPinIds.push_back(pin.id);
					}
				}
				node.inputs.erase(
					std::remove_if(node.inputs.begin(), node.inputs.end(), [](const NodePin& pin) {
						return IsLegacyOutputPin(pin.name);
					}),
					node.inputs.end());
				material.graph.links.erase(
					std::remove_if(material.graph.links.begin(), material.graph.links.end(), [&removedPinIds](const NodeLink& link) {
						return std::find(removedPinIds.begin(), removedPinIds.end(), link.toPinId) != removedPinIds.end() ||
							   std::find(removedPinIds.begin(), removedPinIds.end(), link.fromPinId) != removedPinIds.end();
					}),
					material.graph.links.end());
				return;
			}

			ensureInput("Lighting Mode", NodePinTypes::Int);
			ensureInput("BaseColor", NodePinTypes::Color);
			ensureInput("Emissive", NodePinTypes::Color);
			ensureInput("Emissive Intensity", NodePinTypes::Float);
			return;
		}
		Node node;
		node.id		  = material.graph.AllocateId();
		node.type	  = "Output";
		node.title	  = "Output";
		node.position = {420.0f, 120.0f};
		node.inputs.push_back({material.graph.AllocateId(), "Surface", NodePinKind::Input, ShaderGraphPinTypes::Material});
		material.graph.nodes.push_back(std::move(node));
	}

	bool MaterialNodeEditorPanel::EnsureTextureSampleNodePins(MaterialAsset& material) {
		bool changed = false;
		bool hasObjectTexture = false;
		for(Node& node : material.graph.nodes) {
			if(node.type == "ObjectTexture") {
				if(!hasObjectTexture) {
					hasObjectTexture = true;
				} else if(GetGuidProperty(node, "textureGuid").isValid()) {
					const Guid guid = GetGuidProperty(node, "textureGuid");
					node.type = "Texture2D";
					node.title = "Texture2D";
					if(material.objectTextureGuid == guid) {
						material.objectTextureGuid = Guid::Empty();
					}
					changed = true;
				}
			}

			if(node.type != "TextureSample") continue;

			const bool hasValueOutput = std::any_of(node.outputs.begin(), node.outputs.end(), [](const NodePin& pin) {
				return pin.name == "Value" && pin.type == NodePinTypes::Float;
			});
			if(!hasValueOutput) {
				node.outputs.push_back({material.graph.AllocateId(), "Value", NodePinKind::Output, NodePinTypes::Float});
				changed = true;
			}
		}
		return changed;
	}

	Vector4 MaterialNodeEditorPanel::EvaluateColor(const MaterialAsset& material, int32_t inputPinId, const Vector4& fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->type != NodePinTypes::Color) return fallback;
			if(fromNode->type == "Color") return GetColorProperty(*fromNode, "value", fallback);
			if(fromNode->type == "TextureSample") return {1, 1, 1, 1};
			if(fromNode->type == "NoiseTexture") return {0.5f, 0.5f, 0.5f, 1.0f};
			if(fromNode->type == "MultiplyColor") {
				return EvaluateColor(material, fromNode->inputs[0].id, {1, 1, 1, 1}) *
					   EvaluateColor(material, fromNode->inputs[1].id, {1, 1, 1, 1});
			}
		}
		return fallback;
	}

	float MaterialNodeEditorPanel::EvaluateFloat(const MaterialAsset& material, int32_t inputPinId, float fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->type != NodePinTypes::Float) return fallback;
			if(fromNode->type == "Float" || fromNode->type == "Shininess" || fromNode->type == "Roughness") return fromNode->GetProperty<float>("value", fallback);
			if(fromNode->type == "TextureSample") return 0.5f;
			if(fromNode->type == "NoiseTexture") return 0.5f;
			if(fromNode->type == "UVX" || fromNode->type == "UVY" || fromNode->type == "Time" ||
			   fromNode->type == "WorldPositionX" || fromNode->type == "WorldPositionY" || fromNode->type == "WorldPositionZ" ||
			   fromNode->type == "WorldNormalX" || fromNode->type == "WorldNormalY" || fromNode->type == "WorldNormalZ" ||
			   fromNode->type == "ViewDirectionX" || fromNode->type == "ViewDirectionY" || fromNode->type == "ViewDirectionZ") {
				return fallback;
			}
			if(fromNode->type == "AddFloat") {
				return EvaluateFloat(material, fromNode->inputs[0].id, 0.0f) +
					   EvaluateFloat(material, fromNode->inputs[1].id, 0.0f);
			}
			if(fromNode->type == "SubtractFloat") {
				return EvaluateFloat(material, fromNode->inputs[0].id, 0.0f) -
					   EvaluateFloat(material, fromNode->inputs[1].id, 0.0f);
			}
			if(fromNode->type == "MultiplyFloat") {
				return EvaluateFloat(material, fromNode->inputs[0].id, 1.0f) *
					   EvaluateFloat(material, fromNode->inputs[1].id, 1.0f);
			}
			if(fromNode->type == "DivideFloat") {
				const float b = EvaluateFloat(material, fromNode->inputs[1].id, 1.0f);
				const float denominator = std::abs(b) < 0.0001f ? (b < 0.0f ? -0.0001f : 0.0001f) : b;
				return EvaluateFloat(material, fromNode->inputs[0].id, fallback) / denominator;
			}
			if(fromNode->type == "PowerFloat") {
				return std::pow(
					(std::max)(EvaluateFloat(material, fromNode->inputs[0].id, 1.0f), 0.0f),
					EvaluateFloat(material, fromNode->inputs[1].id, 1.0f));
			}
			if(fromNode->type == "MinFloat") {
				return (std::min)(
					EvaluateFloat(material, fromNode->inputs[0].id, fallback),
					EvaluateFloat(material, fromNode->inputs[1].id, fallback));
			}
			if(fromNode->type == "MaxFloat") {
				return (std::max)(
					EvaluateFloat(material, fromNode->inputs[0].id, fallback),
					EvaluateFloat(material, fromNode->inputs[1].id, fallback));
			}
			if(fromNode->type == "LerpFloat") {
				const float a = EvaluateFloat(material, fromNode->inputs[0].id, fallback);
				const float b = EvaluateFloat(material, fromNode->inputs[1].id, fallback);
				const float t = std::clamp(EvaluateFloat(material, fromNode->inputs[2].id, 0.0f), 0.0f, 1.0f);
				return a + (b - a) * t;
			}
			if(fromNode->type == "SaturateFloat") {
				return std::clamp(EvaluateFloat(material, fromNode->inputs[0].id, fallback), 0.0f, 1.0f);
			}
			if(fromNode->type == "FracFloat") {
				const float value = EvaluateFloat(material, fromNode->inputs[0].id, fallback);
				return value - std::floor(value);
			}
			if(fromNode->type == "OneMinusFloat") {
				return 1.0f - EvaluateFloat(material, fromNode->inputs[0].id, fallback);
			}
			if(fromNode->type == "StepFloat") {
				const float edge = EvaluateFloat(material, fromNode->inputs[0].id, 0.5f);
				const float value = EvaluateFloat(material, fromNode->inputs[1].id, fallback);
				return value < edge ? 0.0f : 1.0f;
			}
			if(fromNode->type == "SmoothstepFloat") {
				const float edge0 = EvaluateFloat(material, fromNode->inputs[0].id, 0.4f);
				const float edge1 = EvaluateFloat(material, fromNode->inputs[1].id, 0.6f);
				const float value = EvaluateFloat(material, fromNode->inputs[2].id, fallback);
				const float width = std::abs(edge1 - edge0) < 0.0001f ? 0.0001f : edge1 - edge0;
				const float t = std::clamp((value - edge0) / width, 0.0f, 1.0f);
				return t * t * (3.0f - 2.0f * t);
			}
		}
		return fallback;
	}

	bool MaterialNodeEditorPanel::EvaluateBool(const MaterialAsset& material, int32_t inputPinId, bool fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->type != NodePinTypes::Bool) return fallback;
			if(fromNode->type == "Reflect") return fromNode->GetProperty<bool>("value", fallback);
		}
		return fallback;
	}

	int32_t MaterialNodeEditorPanel::EvaluateInt(const MaterialAsset& material, int32_t inputPinId, int32_t fallback) const {
		for(const auto& link : material.graph.links) {
			if(link.toPinId != inputPinId) continue;
			const Node*	   fromNode = nullptr;
			const NodePin* fromPin	= material.graph.FindPin(link.fromPinId, &fromNode);
			if(!fromNode || !fromPin || fromPin->type != NodePinTypes::Int) return fallback;
			if(fromNode->type == "LightingMode") {
				const int32_t base = std::clamp(fromNode->GetProperty<int32_t>("value", fallback), 0, kLightingModeCount - 1);
				if(!fromNode->inputs.empty()) {
					return EvaluateInt(material, fromNode->inputs[0].id, base);
				}
				return base;
			}
			if(IsLightingNode(fromNode->type)) {
				return LightingModeFromNodeType(fromNode->type, fallback);
			}
		}
		return fallback;
	}

	void MaterialNodeEditorPanel::ApplyToonLightingNode(MaterialAsset& material, const Node& node) const {
		if(node.type != "ToonLighting") return;
		material.toonHighlightColor = GetColorProperty(node, "toonHighlightColor", material.toonHighlightColor);
		material.toonBaseColor = GetColorProperty(node, "toonBaseColor", material.toonBaseColor);
		material.toonMidShadowColor = GetColorProperty(node, "toonMidShadowColor", material.toonMidShadowColor);
		material.toonShadowColor = GetColorProperty(node, "toonShadowColor", material.toonShadowColor);
		material.toonThreshold1 = GetFloatProperty(node, "toonThreshold1", material.toonThreshold1);
		material.toonThreshold2 = GetFloatProperty(node, "toonThreshold2", material.toonThreshold2);
		material.toonThreshold3 = GetFloatProperty(node, "toonThreshold3", material.toonThreshold3);
		material.toonEdgeSoftness = GetFloatProperty(node, "toonEdgeSoftness", material.toonEdgeSoftness);
		material.toonShadeStep = material.toonThreshold1;
		material.toonShadeFeather = material.toonEdgeSoftness;
		material.toonBaseStep = material.toonThreshold2;
		material.toonBaseFeather = material.toonEdgeSoftness;
		material.toonSpecularThreshold = GetFloatProperty(node, "toonSpecularThreshold", material.toonSpecularThreshold);
		material.toonSpecularSoftness = GetFloatProperty(node, "toonSpecularSoftness", material.toonSpecularSoftness);
		material.toonSpecularIntensity = GetFloatProperty(node, "toonSpecularIntensity", material.toonSpecularIntensity);
	}

	void MaterialNodeEditorPanel::Evaluate(MaterialAsset& material) {
		MaterialGraphCompiler::Compile(material);
	}

	void MaterialNodeEditorPanel::Save(MaterialAsset& material) {
		Evaluate(material);
		for(auto* rec : AssetDatabase::GetInstance()->GetView()) {
			if(rec && rec->type == AssetType::Material && rec->guid == material.GetGuid()) {
				AssetManager::GetInstance()->GetDataAssetManager()->SaveAsset(material, rec->sourcePath);
				return;
			}
		}
		EngineLogger::GetInstance().Add(
			LogLevel::Error,
			LogCategory::Asset,
			"Material save failed because its asset record was not found: " + material.GetName(),
			"MaterialNodeEditorPanel");
	}

	bool MaterialNodeEditorPanel::EnsurePreviewResources() {
		if(previewInitialized_) return true;

		auto device = GraphicsGroup::GetInstance()->GetDevice();
		if(!device) return false;

		previewRtv_ = DescriptorAllocator::Allocate(DescriptorUsage::Rtv);
		previewDsv_ = DescriptorAllocator::Allocate(DescriptorUsage::Dsv);
		if(!previewRtv_.IsValid() || !previewDsv_.IsValid()) return false;

		previewTarget_ = std::make_unique<OffscreenRenderTarget>();
		previewTarget_->SetRenderTargetType(RenderTargetType::Offscreen);
		previewTarget_->Initialize(device.Get(), 256, 256, DXGI_FORMAT_R8G8B8A8_UNORM, previewRtv_, previewDsv_);
		previewMaterialBuffer_.Initialize(device);

		previewInitialized_ = true;
		return true;
	}

	bool MaterialNodeEditorPanel::EnsurePreviewPipeline(const MaterialGraphRuntimeShader& shader) {
		if(previewPipeline_ && previewPipelineHash_ == shader.hash) return true;
		if(!shader.pixelShader) return false;

		try {
			ShaderCompiler compiler;
			compiler.InitializeDXC();
			PsoFactory factory(&compiler);

			GraphicsPipelineDesc desc;
			desc.VS(L"CopyImage.VS.hlsl")
				.BlendNone()
				.CullNone()
				.DepthEnable(false)
				.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
				.Samples(1);
			desc.inputElems_.clear();
			desc.root_
				.AllowIA()
				.CBV(0, D3D12_SHADER_VISIBILITY_PIXEL)
				.SRVTable(0, 1, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_SHADER_VISIBILITY_PIXEL)
				.SRVTable(9, 8, D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_SHADER_VISIBILITY_PIXEL)
				.SamplerWrapLinear(0);

			previewPipeline_ = factory.CreateWithPixelShaderBlob(desc, shader.pixelShader);
			previewPipelineHash_ = shader.hash;
		} catch(const std::exception& e) {
			graphStatusMessage_ = std::string("Preview PSO creation failed: ") + e.what();
			graphStatusIsError_ = true;
			previewPipeline_.reset();
			previewPipelineHash_ = 0;
			return false;
		}

		return previewPipeline_ != nullptr;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE MaterialNodeEditorPanel::ResolvePreviewTexture(const MaterialAsset& material) const {
		auto* textureManager = AssetManager::GetInstance()->GetTextureManager();
		const Guid graphTextureGuid = FindGraphTextureGuid(material);
		if(graphTextureGuid.isValid()) {
			D3D12_GPU_DESCRIPTOR_HANDLE handle = textureManager->LoadTexture(graphTextureGuid);
			if(handle.ptr) return handle;
		}
		return textureManager->LoadTexture("textures/white1x1.dds");
	}

	D3D12_GPU_DESCRIPTOR_HANDLE MaterialNodeEditorPanel::ResolvePreviewTextureTable(const MaterialAsset& material) {
		constexpr uint32_t kMaxGraphTextures = 8;
		auto* textureManager = AssetManager::GetInstance()->GetTextureManager();
		ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
		if(!textureManager || !device) return {};

		if(!previewTextureTable_.IsValid()) {
			previewTextureTable_ = DescriptorAllocator::AllocateRange(DescriptorUsage::CbvSrvUav, kMaxGraphTextures);
		}

		const UINT descriptorSize = DescriptorAllocator::GetDescriptorSize(DescriptorUsage::CbvSrvUav);
		for(uint32_t i = 0; i < kMaxGraphTextures; ++i) {
			D3D12_CPU_DESCRIPTOR_HANDLE dest = previewTextureTable_.cpu;
			dest.ptr += static_cast<SIZE_T>(i) * descriptorSize;
			textureManager->WriteSrvTo("textures/white1x1.dds", dest);
		}

		uint32_t slot = 0;
		for(const Node& node : material.graph.nodes) {
			if(!IsGraphTextureNodeType(node.type)) continue;
			if(slot >= kMaxGraphTextures) break;

			const Guid guid = ResolveGraphTextureGuid(material, node);
			if(guid.isValid()) {
				D3D12_CPU_DESCRIPTOR_HANDLE dest = previewTextureTable_.cpu;
				dest.ptr += static_cast<SIZE_T>(slot) * descriptorSize;
				textureManager->WriteSrvTo(guid, dest);
			}
			++slot;
		}

		return previewTextureTable_.gpu;
	}

	Material MaterialNodeEditorPanel::BuildPreviewMaterial(const MaterialAsset& material) const {
		Material data{};
		data.color = material.color;
		data.lightingMode = material.lightingMode;
		data.shininess = material.shininess;
		data.isReflect = material.isReflect ? 1 : 0;
		data.envirometCoefficient = material.envirometCoefficient;
		data.roughness = material.roughness;
		data.toonHighlightColor = material.toonHighlightColor;
		data.toonBaseColor = material.toonBaseColor;
		data.toonMidShadowColor = material.toonMidShadowColor;
		data.toonShadowColor = material.toonShadowColor;
		data.toonBaseStep = material.toonBaseStep;
		data.toonBaseFeather = material.toonBaseFeather;
		data.toonShadeStep = material.toonShadeStep;
		data.toonShadeFeather = material.toonShadeFeather;
		data.toonSpecularThreshold = material.toonSpecularThreshold;
		data.toonSpecularSoftness = material.toonSpecularSoftness;
		data.toonSpecularIntensity = material.toonSpecularIntensity;
		data.emissiveColor = material.emissiveColor;
		data.emissiveIntensity = material.emissiveIntensity;
		data.rimColor = material.rimColor;
		data.rimIntensity = material.rimIntensity;
		data.rimPower = material.rimPower;
		data.useNormalMap = material.useNormalMap ? 1 : 0;
		data.normalMapStrength = material.normalMapStrength;
		data.normalMapFlipY = material.normalMapFlipY ? 1 : 0;
		data.uvTransform = material.uvTransform;
		data.pad3 = ClockManager::GetInstance()->GetTotalTime();
		return data;
	}
} // namespace CalyxEngine
