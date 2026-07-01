#pragma once

// engine
#include <Engine/Editor/NodeEditor/NodeGraph.h>

// c++
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		汎用Node Graphの検証結果
	/////////////////////////////////////////////////////////////////////////////////////////
	struct NodeGraphValidationResult {
		bool ok = true;
		std::vector<std::string> messages;

		void Error(std::string message) {
			ok = false;
			messages.push_back(std::move(message));
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		全Node Graphで共通となる構造検証
	/////////////////////////////////////////////////////////////////////////////////////////
	class NodeGraphValidator {
	public:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		リンク、入力数、循環参照を検証
		/////////////////////////////////////////////////////////////////////////////////////////
		static NodeGraphValidationResult Validate(const NodeGraph& graph) {
			NodeGraphValidationResult result;

			// 全リンクの参照先、方向、型を検証する。
			ValidateLinks(graph, result);

			// 一つの入力へ複数リンクされていないか検証する。
			ValidateSingleInputLinks(graph, result);

			// グラフが循環していないか検証する。
			ValidateNoCycles(graph, result);
			return result;
		}

	private:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		リンクの参照と型を検証
		/////////////////////////////////////////////////////////////////////////////////////////
		static void ValidateLinks(const NodeGraph& graph, NodeGraphValidationResult& result) {
			for(const NodeLink& link : graph.links) {
				const Node* fromNode = nullptr;
				const Node* toNode = nullptr;
				const NodePin* fromPin = graph.FindPin(link.fromPinId, &fromNode);
				const NodePin* toPin = graph.FindPin(link.toPinId, &toNode);
				if(!fromPin || !toPin || !fromNode || !toNode) {
					result.Error("Broken link: missing node or pin.");
					continue;
				}
				if(fromPin->kind != NodePinKind::Output || toPin->kind != NodePinKind::Input) {
					result.Error("Invalid link direction: " + fromNode->title + "." + fromPin->name + " -> " + toNode->title + "." + toPin->name);
				}
				if(fromPin->type != toPin->type) {
					result.Error("Type mismatch: " + fromNode->title + "." + fromPin->name + " -> " + toNode->title + "." + toPin->name);
				}
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		入力ピンへの多重リンクを検証
		/////////////////////////////////////////////////////////////////////////////////////////
		static void ValidateSingleInputLinks(const NodeGraph& graph, NodeGraphValidationResult& result) {
			std::unordered_map<int32_t, int32_t> incomingCount;
			for(const NodeLink& link : graph.links) ++incomingCount[link.toPinId];

			for(const auto& [pinId, count] : incomingCount) {
				if(count <= 1) continue;
				const Node* node = nullptr;
				const NodePin* pin = graph.FindPin(pinId, &node);
				result.Error(node && pin
					? "Input has multiple links: " + node->title + "." + pin->name
					: "Input has multiple links: missing pin.");
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		有向グラフの循環参照を検証
		/////////////////////////////////////////////////////////////////////////////////////////
		static void ValidateNoCycles(const NodeGraph& graph, NodeGraphValidationResult& result) {
			std::unordered_map<int32_t, std::vector<int32_t>> edges;
			for(const NodeLink& link : graph.links) {
				const Node* fromNode = nullptr;
				const Node* toNode = nullptr;
				if(!graph.FindPin(link.fromPinId, &fromNode) || !graph.FindPin(link.toPinId, &toNode) || !fromNode || !toNode) continue;
				edges[fromNode->id].push_back(toNode->id);
			}

			std::unordered_set<int32_t> visiting;
			std::unordered_set<int32_t> visited;
			std::function<bool(int32_t)> hasCycle = [&](int32_t nodeId) {
				if(visiting.contains(nodeId)) return true;
				if(visited.contains(nodeId)) return false;
				visiting.insert(nodeId);
				for(int32_t next : edges[nodeId]) if(hasCycle(next)) return true;
				visiting.erase(nodeId);
				visited.insert(nodeId);
				return false;
			};

			for(const Node& node : graph.nodes) {
				if(!hasCycle(node.id)) continue;
				result.Error("Graph contains a cycle.");
				return;
			}
		}
	};
}
