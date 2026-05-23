#pragma once

#include <Engine\Foundation\Math\Vector2.h>
#include <Engine\Foundation\Math\Vector4.h>
#include <externals\nlohmann\json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace CalyxEngine {
	enum class NodePinKind : int32_t { Input, Output };
	enum class NodeValueType : int32_t { None, Float, Color, Bool, Int, Material, Texture2D, Float2, Float3, Float4 };

	struct NodePin {
		int32_t id = 0;
		std::string name;
		NodePinKind kind = NodePinKind::Input;
		NodeValueType valueType = NodeValueType::None;
	};

	struct Node {
		int32_t id = 0;
		std::string type;
		std::string title;
		Vector2 position{};
		std::vector<NodePin> inputs;
		std::vector<NodePin> outputs;
		float floatValue = 0.0f;
		int32_t intValue = 0;
		Vector4 colorValue = {1, 1, 1, 1};
		bool boolValue = false;
		nlohmann::json properties = nlohmann::json::object();
	};

	struct NodeLink {
		int32_t id = 0;
		int32_t fromPinId = 0;
		int32_t toPinId = 0;
	};

	struct NodeGraph {
		std::vector<Node> nodes;
		std::vector<NodeLink> links;
		int32_t nextId = 1;

		int32_t AllocateId() { return nextId++; }
		Node* FindNode(int32_t id);
		NodePin* FindPin(int32_t pinId, Node** owner = nullptr);
		const NodePin* FindPin(int32_t pinId, const Node** owner = nullptr) const;
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
