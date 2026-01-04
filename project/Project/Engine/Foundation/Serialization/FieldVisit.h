#pragma once

#include <externals\nlohmann\json.hpp>

// 書き
struct JsonWriter {
	nlohmann::json& j;
	template <class U>
	void field(const char* name, const U& value) { j[name] = value; }
};

// 読み
struct JsonReader {
	const nlohmann::json& j;
	template <class U>
	void field(const char* name, U& value) const {
		if(auto it = j.find(name); it != j.end()) it->get_to(value); // 欠損→既定値保持
	}
};

template <class T>
inline void SerializeData(json& root, const T& obj) {
	json&	   d = root["data"];
	JsonWriter w{d};
	obj.visit_fields(w); // T が visit_fields を持つこと
}

template <class T>
inline void DeserializeData(const nlohmann::json& root, T& obj) {
	auto it = root.find("data");
	if(it == root.end()) return;
	const json& d = *it;
	JsonReader	r{d};
	obj.visit_fields(r); // 書きと同じ列挙で安全に復元
}