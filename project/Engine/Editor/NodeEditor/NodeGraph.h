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

	/*-----------------------------------------------------------------------------------------
	 * NodePin
	 * - 汎用Node Graphの入出力端子を表すデータ構造
	 * - 一意ID、表示名、入出力方向、接続互換性を判定する型IDを保持する
	 *---------------------------------------------------------------------------------------*/
	struct NodePin {
		int32_t id = 0;
		std::string name;
		NodePinKind kind = NodePinKind::Input;
		std::string type = std::string(NodePinTypes::None);

		NodePin() = default;
		NodePin(int32_t pinId, std::string pinName, NodePinKind pinKind, std::string_view pinType)
			: id(pinId), name(std::move(pinName)), kind(pinKind), type(pinType) {}
	};

	/*-----------------------------------------------------------------------------------------
	 * Node
	 * - 用途に依存しないNode本体を表すデータ構造
	 * - 入出力ピン、Canvas座標、用途固有プロパティを保持する
	 * - Node固有の評価処理やEditor描画は管理しない
	 *---------------------------------------------------------------------------------------*/
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

	/*-----------------------------------------------------------------------------------------
	 * NodeLink
	 * - 出力ピンから入力ピンへの接続を表すデータ構造
	 * - Link自身のIDと接続先ピンIDを保持する
	 *---------------------------------------------------------------------------------------*/
	struct NodeLink {
		int32_t id = 0;
		int32_t fromPinId = 0;
		int32_t toPinId = 0;
	};

	/*-----------------------------------------------------------------------------------------
	 * NodeGraph
	 * - NodeとLinkを所有する汎用グラフデータ構造
	 * - 要素検索とNode、Pin、Linkで共有する一意IDの発行を担当する
	 * - 用途固有の接続規則や実行順序は管理しない
	 *---------------------------------------------------------------------------------------*/
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
