#include "SceneObjectRegistry.h"

SceneObjectRegistry& SceneObjectRegistry::Get(){
	static SceneObjectRegistry inst;
	return inst;
}
void SceneObjectRegistry::Register(std::string_view name, SceneObjectFactory factory){
	SceneObjectClassDesc desc;
	desc.typeName	 = std::string(name);
	desc.displayName = desc.typeName;
	desc.factory	 = factory;
	Register(desc);
}

void SceneObjectRegistry::Register(
	const char* typeName,
	const char* displayName,
	ObjectType objectType,
	const char* iconPath,
	bool placeable,
	bool prefabEditable,
	bool prefabRoot,
	SceneObjectFactory factory){

	SceneObjectClassDesc desc;
	desc.typeName = typeName ? typeName : "";
	desc.displayName = displayName ? displayName : desc.typeName;
	desc.objectType = objectType;
	desc.iconPath = iconPath ? iconPath : "";
	desc.placeable = placeable;
	desc.prefabEditable = prefabEditable;
	desc.prefabRoot = prefabRoot;
	desc.factory = factory;
	Register(desc);
}

void SceneObjectRegistry::Register(const SceneObjectClassDesc& desc){
	if(desc.typeName.empty()) {
		return;
	}

	SceneObjectClassDesc stored;
	stored.typeName = desc.typeName;
	stored.displayName = desc.displayName.empty() ? desc.typeName : desc.displayName;
	stored.objectType = desc.objectType;
	stored.iconPath = desc.iconPath;
	stored.placeable = desc.placeable;
	stored.prefabEditable = desc.prefabEditable;
	stored.prefabRoot = desc.prefabRoot;
	stored.factory = desc.factory;

	const std::string key = stored.typeName;
	auto			  it  = table_.find(key);
	if(it == table_.end()) {
		table_.emplace(key, std::move(stored));
		++revision_;
		return;
	}

	auto& current = it->second;
	current.displayName = std::move(stored.displayName);
	current.objectType	= stored.objectType;
	current.iconPath	= std::move(stored.iconPath);
	current.placeable	= stored.placeable;
	current.prefabEditable = stored.prefabEditable;
	current.prefabRoot = stored.prefabRoot;
	if(stored.factory) {
		current.factory = stored.factory;
	}
	++revision_;
}
std::shared_ptr<SceneObject> SceneObjectRegistry::Create(std::string_view name) const{
	auto it = table_.find(std::string(name));
	if (it == table_.end() || !it->second.factory)
		throw std::runtime_error("Unknown SceneObject type: " + std::string(name));
	auto object = it->second.factory();
	if(object) {
		object->SetTypeName(it->second.typeName);
	}
	return object;
}
std::vector<std::string> SceneObjectRegistry::ListTypes() const{
	std::vector<std::string> out;
	for (auto& [k, _] : table_) out.push_back(k);
	return out;
}

std::vector<SceneObjectClassDesc const*> SceneObjectRegistry::ListPlaceableTypes() const{
	std::vector<SceneObjectClassDesc const*> out;
	for(const auto& [_, desc] : table_) {
		if(desc.placeable && desc.factory) {
			out.push_back(&desc);
		}
	}
	return out;
}

std::vector<SceneObjectClassDesc const*> SceneObjectRegistry::ListPrefabEditableTypes() const{
	std::vector<SceneObjectClassDesc const*> out;
	for(const auto& [_, desc] : table_) {
		if(desc.prefabEditable && desc.factory) {
			out.push_back(&desc);
		}
	}
	return out;
}

std::vector<SceneObjectClassDesc const*> SceneObjectRegistry::ListPrefabRootTypes() const{
	std::vector<SceneObjectClassDesc const*> out;
	for(const auto& [_, desc] : table_) {
		if(desc.prefabRoot && desc.factory) {
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

std::size_t SceneObjectRegistry::GetRevision() const {
	return revision_;
}
