#pragma once

// engine
#include <Engine/Editor/NodeEditor/NodeGraph.h>

// c++
#include <array>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace CalyxEngine {
	/////////////////////////////////////////////////////////////////////////////////////////
	//		Node型に定義されるPin情報
	/////////////////////////////////////////////////////////////////////////////////////////
	struct NodePinDefinition {
		std::string name;
		NodePinKind kind = NodePinKind::Input;
		std::string type = std::string(NodePinTypes::None);

		NodePinDefinition() = default;
		NodePinDefinition(std::string pinName, std::string_view pinType, NodePinKind pinKind)
			: name(std::move(pinName)), kind(pinKind), type(pinType) {}
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		Node生成とEditor表示に使用するNode型定義
	/////////////////////////////////////////////////////////////////////////////////////////
	struct NodeTypeDefinition {
		std::string type;
		std::string title;
		std::string category;
		std::vector<NodePinDefinition> pins;
		nlohmann::json defaultProperties = nlohmann::json::object();
		std::array<float, 4> headerColor = {0.20f, 0.24f, 0.30f, 1.0f};

		NodeTypeDefinition() = default;
		NodeTypeDefinition(std::string nodeType, std::string nodeTitle, std::vector<NodePinDefinition> nodePins)
			: type(std::move(nodeType)), title(std::move(nodeTitle)), pins(std::move(nodePins)) {}
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		用途別Node型を保持する汎用Registry
	/////////////////////////////////////////////////////////////////////////////////////////
	class NodeTypeRegistry {
	public:
		/////////////////////////////////////////////////////////////////////////////////////////
		//		Node型を登録
		/////////////////////////////////////////////////////////////////////////////////////////
		bool Register(NodeTypeDefinition definition) {
			if(definition.type.empty() || definitionsByType_.contains(definition.type)) return false;

			// 検索Mapはvectorへ格納した定義のIndexを保持する。
			const size_t index = definitions_.size();
			definitionsByType_.emplace(definition.type, index);
			definitions_.push_back(std::move(definition));
			return true;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		型IDからNode定義を検索
		/////////////////////////////////////////////////////////////////////////////////////////
		const NodeTypeDefinition* Find(std::string_view type) const {
			const auto it = definitionsByType_.find(std::string(type));
			return it == definitionsByType_.end() ? nullptr : &definitions_[it->second];
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		登録定義からNodeを生成
		/////////////////////////////////////////////////////////////////////////////////////////
		Node CreateNode(NodeGraph& graph, std::string_view type, Vector2 position) const {
			const NodeTypeDefinition* definition = Find(type);
			if(!definition) return {};

			// Node本体へ型、表示名、既定プロパティを設定する。
			Node node;
			node.id = graph.AllocateId();
			node.type = definition->type;
			node.title = definition->title;
			node.position = position;
			node.properties = definition->defaultProperties;

			// 定義された順序を維持して入力・出力Pinを生成する。
			for(const NodePinDefinition& pin : definition->pins) {
				NodePin created(graph.AllocateId(), pin.name, pin.kind, pin.type);
				(pin.kind == NodePinKind::Input ? node.inputs : node.outputs).push_back(std::move(created));
			}
			return node;
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		登録順を維持した全Node定義を取得
		/////////////////////////////////////////////////////////////////////////////////////////
		const std::vector<NodeTypeDefinition>& Definitions() const { return definitions_; }

	private:
		std::vector<NodeTypeDefinition> definitions_;
		std::unordered_map<std::string, size_t> definitionsByType_;
	};
}
