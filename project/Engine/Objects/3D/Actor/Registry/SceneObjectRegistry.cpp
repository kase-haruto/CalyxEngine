#include "SceneObjectRegistry.h"

SceneObjectRegistry& SceneObjectRegistry::Get(){
	static SceneObjectRegistry inst;
	return inst;
}
void SceneObjectRegistry::Register(std::string_view name, std::unique_ptr<ISceneCtor>&& c){
	SceneObjectClassDesc desc;
	desc.typeName	 = std::string(name);
	desc.displayName = desc.typeName;
	desc.ctor		 = std::move(c);
	Register(std::move(desc));
}

void SceneObjectRegistry::Register(SceneObjectClassDesc&& desc){
	if(desc.typeName.empty()) {
		return;
	}

	if(desc.displayName.empty()) {
		desc.displayName = desc.typeName;
	}

	const std::string key = desc.typeName;
	auto			  it  = table_.find(key);
	if(it == table_.end()) {
		table_.emplace(key, std::move(desc));
		return;
	}

	auto& current = it->second;
	current.displayName = std::move(desc.displayName);
	current.objectType	= desc.objectType;
	current.iconPath	= std::move(desc.iconPath);
	current.placeable	= desc.placeable;
	current.prefabEditable = desc.prefabEditable;
	current.prefabRoot = desc.prefabRoot;
	if(desc.ctor) {
		current.ctor = std::move(desc.ctor);
	}
}
std::shared_ptr<SceneObject> SceneObjectRegistry::Create(std::string_view name) const{
	auto it = table_.find(std::string(name));
	if (it == table_.end() || !it->second.ctor)
		throw std::runtime_error("Unknown SceneObject type: " + std::string(name));
	return it->second.ctor->New();
}
std::vector<std::string> SceneObjectRegistry::ListTypes() const{
	std::vector<std::string> out;
	for (auto& [k, _] : table_) out.push_back(k);
	return out;
}

std::vector<SceneObjectClassDesc const*> SceneObjectRegistry::ListPlaceableTypes() const{
	std::vector<SceneObjectClassDesc const*> out;
	for(const auto& [_, desc] : table_) {
		if(desc.placeable && desc.ctor) {
			out.push_back(&desc);
		}
	}
	return out;
}

std::vector<SceneObjectClassDesc const*> SceneObjectRegistry::ListPrefabEditableTypes() const{
	std::vector<SceneObjectClassDesc const*> out;
	for(const auto& [_, desc] : table_) {
		if(desc.prefabEditable && desc.ctor) {
			out.push_back(&desc);
		}
	}
	return out;
}

std::vector<SceneObjectClassDesc const*> SceneObjectRegistry::ListPrefabRootTypes() const{
	std::vector<SceneObjectClassDesc const*> out;
	for(const auto& [_, desc] : table_) {
		if(desc.prefabRoot && desc.ctor) {
			out.push_back(&desc);
		}
	}
	return out;
}

const SceneObjectClassDesc* SceneObjectRegistry::Find(std::string_view typeName) const{
	auto it = table_.find(std::string(typeName));
	if(it == table_.end()) {
		return nullptr;
	}
	return &it->second;
}
