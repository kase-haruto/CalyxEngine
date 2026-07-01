#include "NodeGraph.h"

#include <algorithm>
#include <iterator>

namespace CalyxEngine {
	namespace {
		/////////////////////////////////////////////////////////////////////////////////////////
		//		旧NodeValueTypeの数値を汎用型IDへ変換
		/////////////////////////////////////////////////////////////////////////////////////////
		std::string LegacyPinTypeToId(int32_t valueType) {
			constexpr std::string_view types[] = {
				NodePinTypes::None, NodePinTypes::Float, NodePinTypes::Color,
				NodePinTypes::Bool, NodePinTypes::Int, "shader.material",
				"shader.texture2d", NodePinTypes::Float2, NodePinTypes::Float3,
				NodePinTypes::Float4};
			if(valueType < 0 || valueType >= static_cast<int32_t>(std::size(types))) {
				return std::string(NodePinTypes::None);
			}
			return std::string(types[valueType]);
		}
	}

	Node* NodeGraph::FindNode(int32_t id) {
		for(auto& n : nodes) if(n.id == id) return &n;
		return nullptr;
	}

	NodePin* NodeGraph::FindPin(int32_t pinId, Node** owner) {
		for(auto& n : nodes) {
			for(auto& p : n.inputs) if(p.id == pinId) { if(owner) *owner = &n; return &p; }
			for(auto& p : n.outputs) if(p.id == pinId) { if(owner) *owner = &n; return &p; }
		}
		return nullptr;
	}

	const NodePin* NodeGraph::FindPin(int32_t pinId, const Node** owner) const {
		for(const auto& n : nodes) {
			for(const auto& p : n.inputs) if(p.id == pinId) { if(owner) *owner = &n; return &p; }
			for(const auto& p : n.outputs) if(p.id == pinId) { if(owner) *owner = &n; return &p; }
		}
		return nullptr;
	}

	void NodeGraph::EnsureNextId() {
		int32_t maxId = 0;
		for(const auto& n : nodes) {
			maxId = std::max(maxId, n.id);
			for(const auto& p : n.inputs) maxId = std::max(maxId, p.id);
			for(const auto& p : n.outputs) maxId = std::max(maxId, p.id);
		}
		for(const auto& l : links) maxId = std::max(maxId, l.id);
		nextId = std::max(nextId, maxId + 1);
	}

	void to_json(nlohmann::json& j, const NodePin& p) {
		j = {{"id", p.id}, {"name", p.name}, {"kind", static_cast<int32_t>(p.kind)}, {"type", p.type}};
	}
	void from_json(const nlohmann::json& j, NodePin& p) {
		p.id = j.value("id", 0);
		p.name = j.value("name", "");
		p.kind = static_cast<NodePinKind>(j.value("kind", 0));
		// 新形式の型IDを優先し、旧形式は読み込み時に移行する。
		p.type = j.contains("type")
			? j.value("type", std::string(NodePinTypes::None))
			: LegacyPinTypeToId(j.value("valueType", 0));
	}
	void to_json(nlohmann::json& j, const Node& n) {
		j = {{"id", n.id}, {"type", n.type}, {"title", n.title}, {"position", {n.position.x, n.position.y}}, {"inputs", n.inputs}, {"outputs", n.outputs}, {"properties", n.properties}};
	}
	void from_json(const nlohmann::json& j, Node& n) {
		n.id = j.value("id", 0);
		n.type = j.value("type", "");
		n.title = j.value("title", n.type);
		if(auto it = j.find("position"); it != j.end() && it->is_array() && it->size() == 2) n.position = {it->at(0).get<float>(), it->at(1).get<float>()};
		n.inputs = j.value("inputs", std::vector<NodePin>{});
		n.outputs = j.value("outputs", std::vector<NodePin>{});
		if(auto it = j.find("properties"); it != j.end() && it->is_object()) n.properties = *it;

		// 旧Nodeの専用値を汎用propertiesへ移行する。
		if(!n.properties.contains("value")) {
			if(n.type == "Color" && j.contains("colorValue")) {
				n.properties["value"] = j.at("colorValue");
			} else if((n.type == "Float" || n.type == "Float2" || n.type == "Shininess" || n.type == "Roughness") && j.contains("floatValue")) {
				n.properties["value"] = j.at("floatValue");
			} else if((n.type == "LightingMode" || n.type.ends_with("Lighting")) && j.contains("intValue")) {
				n.properties["value"] = j.at("intValue");
			} else if(n.type == "Reflect" && j.contains("boolValue")) {
				n.properties["value"] = j.at("boolValue");
			}
		}
	}
	void to_json(nlohmann::json& j, const NodeLink& l) { j = {{"id", l.id}, {"fromPinId", l.fromPinId}, {"toPinId", l.toPinId}}; }
	void from_json(const nlohmann::json& j, NodeLink& l) {
		l.id = j.value("id", 0);
		l.fromPinId = j.value("fromPinId", 0);
		l.toPinId = j.value("toPinId", 0);
	}
	void to_json(nlohmann::json& j, const NodeGraph& g) { j = {{"nodes", g.nodes}, {"links", g.links}, {"nextId", g.nextId}}; }
	void from_json(const nlohmann::json& j, NodeGraph& g) {
		g.nodes = j.value("nodes", std::vector<Node>{});
		g.links = j.value("links", std::vector<NodeLink>{});
		g.nextId = j.value("nextId", 1);
		g.EnsureNextId();
	}
}
