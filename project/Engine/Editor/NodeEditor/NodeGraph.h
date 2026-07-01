#pragma once

#include <Engine\Foundation\Math\Vector2.h>
#include <externals\nlohmann\json.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CalyxEngine {
	enum class NodePinKind : int32_t { Input, Output };

	/////////////////////////////////////////////////////////////////////////////////////////
	//		汎用Node Graphで使用する組み込みピン型ID
	/////////////////////////////////////////////////////////////////////////////////////////
	namespace NodePinTypes {
		inline constexpr std::string_view None = "core.none";
		inline constexpr std::string_view Float = "core.float";
		inline constexpr std::string_view Color = "core.color";
		inline constexpr std::string_view Bool = "core.bool";
		inline constexpr std::string_view Int = "core.int";
		inline constexpr std::string_view Float2 = "core.float2";
		inline constexpr std::string_view Float3 = "core.float3";
		inline constexpr std::string_view Float4 = "core.float4";
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		Nodeの入出力端子
	/////////////////////////////////////////////////////////////////////////////////////////
	struct NodePin {
		int32_t id = 0;
		std::string name;
		NodePinKind kind = NodePinKind::Input;
		std::string type = std::string(NodePinTypes::None);

		NodePin() = default;
		NodePin(int32_t pinId, std::string pinName, NodePinKind pinKind, std::string_view pinType)
			: id(pinId), name(std::move(pinName)), kind(pinKind), type(pinType) {}
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		用途に依存しないNodeデータ
	/////////////////////////////////////////////////////////////////////////////////////////
	struct Node {
		int32_t id = 0;
		std::string type;
		std::string title;
		Vector2 position{};
		std::vector<NodePin> inputs;
		std::vector<NodePin> outputs;
		nlohmann::json properties = nlohmann::json::object();

		/////////////////////////////////////////////////////////////////////////////////////////
		//		型を限定せずNodeプロパティを取得
		/////////////////////////////////////////////////////////////////////////////////////////
		template<class T>
		T GetProperty(const std::string& key, const T& fallback) const {
			return properties.value(key, fallback);
		}

		/////////////////////////////////////////////////////////////////////////////////////////
		//		型を限定せずNodeプロパティを設定
		/////////////////////////////////////////////////////////////////////////////////////////
		template<class T>
		void SetProperty(const std::string& key, T&& value) {
			properties[key] = std::forward<T>(value);
		}
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		出力ピンから入力ピンへの接続
	/////////////////////////////////////////////////////////////////////////////////////////
	struct NodeLink {
		int32_t id = 0;
		int32_t fromPinId = 0;
		int32_t toPinId = 0;
	};

	/////////////////////////////////////////////////////////////////////////////////////////
	//		NodeとLinkを保持する汎用グラフ
	/////////////////////////////////////////////////////////////////////////////////////////
	struct NodeGraph {
		std::vector<Node> nodes;
		std::vector<NodeLink> links;
		int32_t nextId = 1;

		// Node、Pin、Linkで共有する一意IDを発行する。
		int32_t AllocateId() { return nextId++; }

		// 指定IDのNodeを検索する。
		Node* FindNode(int32_t id);

		// 指定IDのPinと所有Nodeを検索する。
		NodePin* FindPin(int32_t pinId, Node** owner = nullptr);
		const NodePin* FindPin(int32_t pinId, const Node** owner = nullptr) const;

		// 読み込んだIDから次回発行IDを復元する。
		void EnsureNextId();
	};

	void to_json(nlohmann::json& j, const NodePin& p);
	void from_json(const nlohmann::json& j, NodePin& p);
	void to_json(nlohmann::json& j, const Node& n);
	void from_json(const nlohmann::json& j, Node& n);
	void to_json(nlohmann::json& j, const NodeLink& l);
	void from_json(const nlohmann::json& j, NodeLink& l);
	void to_json(nlohmann::json& j, const NodeGraph& g);
	void from_json(const nlohmann::json& j, NodeGraph& g);
}
