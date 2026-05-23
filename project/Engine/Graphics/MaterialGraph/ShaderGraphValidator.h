#pragma once

#include <Engine/Editor/NodeEditor/NodeGraph.h>
#include <Engine/Graphics/MaterialGraph/ShaderReflectionInfo.h>

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CalyxEngine {
	struct ShaderGraphValidationResult {
		bool ok = true;
		std::vector<std::string> messages;

		void Error(std::string message) {
			ok = false;
			messages.push_back(std::move(message));
		}

		void Warning(std::string message) {
			messages.push_back("Warning: " + std::move(message));
		}
	};

	class ShaderGraphValidator {
	public:
		static ShaderGraphValidationResult ValidateMaterialGraph(const NodeGraph& graph) {
			ShaderGraphValidationResult result;
			ValidateLinks(graph, result);
			ValidateSingleInputLinks(graph, result);
			ValidateTextureSamples(graph, result);
			ValidateOutputSurface(graph, result);
			ValidateNoCycles(graph, result);
			if(result.ok && result.messages.empty()) {
				result.messages.push_back("Material graph is valid.");
			}
			return result;
		}

		static ShaderGraphValidationResult ValidateObject3DMaterialShader(const ShaderReflectionInfo& reflection) {
			ShaderGraphValidationResult result;
			RequireResource(reflection, result, "MaterialConstants", ShaderResourceKind::CBuffer, 0);
			RequireResource(reflection, result, "gTexture", ShaderResourceKind::Texture, 0);
			RequireResource(reflection, result, "gSampler", ShaderResourceKind::Sampler, 0);

			const ShaderCBufferLayout* materialConstants = FindCBuffer(reflection, "MaterialConstants");
			if(!materialConstants) {
				result.Error("Missing MaterialConstants cbuffer layout.");
				return result;
			}

			RequireCBufferVariable(*materialConstants, result, "color");
			RequireCBufferVariable(*materialConstants, result, "enableLighting");
			RequireCBufferVariable(*materialConstants, result, "toonBaseStep");
			RequireCBufferVariable(*materialConstants, result, "toonShadeStep");
			RequireCBufferVariable(*materialConstants, result, "toonSpecularIntensity");
			return result;
		}

	private:
		static bool IsMasterNode(const std::string& type) {
			return type == "ToonMaster" || type == "LitMaster" || type == "UnlitMaster";
		}

		static const NodePin* FindInputPin(const Node& node, const std::string& name) {
			for(const NodePin& pin : node.inputs) {
				if(pin.name == name) return &pin;
			}
			return nullptr;
		}

		static const Node* FindNodeByOutputPin(const NodeGraph& graph, int32_t outputPinId) {
			const Node* node = nullptr;
			const NodePin* pin = graph.FindPin(outputPinId, &node);
			return pin && pin->kind == NodePinKind::Output ? node : nullptr;
		}

		static void ValidateLinks(const NodeGraph& graph, ShaderGraphValidationResult& result) {
			for(const NodeLink& link : graph.links) {
				const Node* fromNode = nullptr;
				const Node* toNode = nullptr;
				const NodePin* fromPin = graph.FindPin(link.fromPinId, &fromNode);
				const NodePin* toPin = graph.FindPin(link.toPinId, &toNode);
				if(!fromPin || !toPin) {
					result.Error("Broken link: missing pin.");
					continue;
				}
				if(!fromNode || !toNode) {
					result.Error("Broken link: missing node.");
					continue;
				}
				if(fromPin->kind != NodePinKind::Output || toPin->kind != NodePinKind::Input) {
					result.Error("Invalid link direction: " + fromNode->title + "." + fromPin->name + " -> " + toNode->title + "." + toPin->name);
				}
				if(fromPin->valueType != toPin->valueType) {
					result.Error("Type mismatch: " + fromNode->title + "." + fromPin->name + " -> " + toNode->title + "." + toPin->name);
				}
			}
		}

		static void ValidateSingleInputLinks(const NodeGraph& graph, ShaderGraphValidationResult& result) {
			std::unordered_map<int32_t, int32_t> incomingCount;
			for(const NodeLink& link : graph.links) {
				incomingCount[link.toPinId]++;
			}
			for(const auto& [pinId, count] : incomingCount) {
				if(count <= 1) continue;
				const Node* node = nullptr;
				const NodePin* pin = graph.FindPin(pinId, &node);
				if(node && pin) {
					result.Error("Input has multiple links: " + node->title + "." + pin->name);
				} else {
					result.Error("Input has multiple links: missing pin.");
				}
			}
		}

		static void ValidateTextureSamples(const NodeGraph& graph, ShaderGraphValidationResult& result) {
			for(const Node& node : graph.nodes) {
				if(node.type != "TextureSample") continue;
				const NodePin* texturePin = FindInputPin(node, "Texture");
				if(!texturePin) {
					result.Error("Texture Sample is missing Texture input.");
					continue;
				}
				const bool hasTextureLink = std::any_of(graph.links.begin(), graph.links.end(), [texturePin](const NodeLink& link) {
					return link.toPinId == texturePin->id;
				});
				if(!hasTextureLink) {
					result.Warning("Texture Sample.Texture is not connected. Connect Object Texture or Texture2D for a valid material graph.");
				}
			}
		}

		static void ValidateOutputSurface(const NodeGraph& graph, ShaderGraphValidationResult& result) {
			const Node* output = nullptr;
			for(const Node& node : graph.nodes) {
				if(node.type == "Output") {
					output = &node;
					break;
				}
			}
			if(!output) {
				result.Error("Missing Output node.");
				return;
			}

			const NodePin* surface = FindInputPin(*output, "Surface");
			if(!surface) {
				result.Error("Output.Surface pin is missing.");
				return;
			}

			int surfaceLinkCount = 0;
			const Node* master = nullptr;
			for(const NodeLink& link : graph.links) {
				if(link.toPinId != surface->id) continue;
				surfaceLinkCount++;
				master = FindNodeByOutputPin(graph, link.fromPinId);
			}

			if(surfaceLinkCount == 0) {
				result.Warning("Output.Surface is not connected. Legacy output pins will be used.");
				return;
			}
			if(surfaceLinkCount > 1) {
				result.Error("Output.Surface has multiple links.");
				return;
			}
			if(!master || !IsMasterNode(master->type)) {
				result.Error("Output.Surface must be connected to Toon/Lit/Unlit Master.");
			}
		}

		static void ValidateNoCycles(const NodeGraph& graph, ShaderGraphValidationResult& result) {
			std::unordered_map<int32_t, std::vector<int32_t>> edges;
			for(const NodeLink& link : graph.links) {
				const Node* fromNode = nullptr;
				const Node* toNode = nullptr;
				const NodePin* fromPin = graph.FindPin(link.fromPinId, &fromNode);
				const NodePin* toPin = graph.FindPin(link.toPinId, &toNode);
				if(!fromPin || !toPin || !fromNode || !toNode) continue;
				edges[fromNode->id].push_back(toNode->id);
			}

			std::unordered_set<int32_t> visiting;
			std::unordered_set<int32_t> visited;
			std::function<bool(int32_t)> dfs = [&](int32_t nodeId) {
				if(visiting.contains(nodeId)) return true;
				if(visited.contains(nodeId)) return false;
				visiting.insert(nodeId);
				for(int32_t next : edges[nodeId]) {
					if(dfs(next)) return true;
				}
				visiting.erase(nodeId);
				visited.insert(nodeId);
				return false;
			};

			for(const Node& node : graph.nodes) {
				if(dfs(node.id)) {
					result.Error("Graph contains a cycle.");
					return;
				}
			}
		}

		static const ShaderResourceBinding* FindResource(
			const ShaderReflectionInfo& reflection,
			const std::string& name) {
			auto it = std::find_if(reflection.resources.begin(), reflection.resources.end(), [&name](const ShaderResourceBinding& binding) {
				return binding.name == name;
			});
			return it == reflection.resources.end() ? nullptr : &(*it);
		}

		static const ShaderCBufferLayout* FindCBuffer(
			const ShaderReflectionInfo& reflection,
			const std::string& name) {
			auto it = std::find_if(reflection.cbuffers.begin(), reflection.cbuffers.end(), [&name](const ShaderCBufferLayout& layout) {
				return layout.name == name;
			});
			return it == reflection.cbuffers.end() ? nullptr : &(*it);
		}

		static void RequireResource(
			const ShaderReflectionInfo& reflection,
			ShaderGraphValidationResult& result,
			const std::string& name,
			ShaderResourceKind kind,
			uint32_t bindPoint) {
			const ShaderResourceBinding* binding = FindResource(reflection, name);
			if(!binding) {
				result.Error("Missing shader resource: " + name);
				return;
			}
			if(binding->kind != kind) {
				result.Error("Unexpected resource kind: " + name);
			}
			if(binding->bindPoint != bindPoint) {
				result.Error("Unexpected bind point for " + name);
			}
		}

		static void RequireCBufferVariable(
			const ShaderCBufferLayout& layout,
			ShaderGraphValidationResult& result,
			const std::string& name) {
			const auto it = std::find_if(layout.variables.begin(), layout.variables.end(), [&name](const ShaderCBufferVariable& variable) {
				return variable.name == name;
			});
			if(it == layout.variables.end()) {
				result.Error("Missing cbuffer variable: " + layout.name + "." + name);
			}
		}
	};
}
