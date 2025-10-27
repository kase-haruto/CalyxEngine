#include "SceneObjectRegistry.h"

SceneObjectRegistry& SceneObjectRegistry::Get(){
	static SceneObjectRegistry inst;
	return inst;
}
void SceneObjectRegistry::Register(std::string_view name, std::unique_ptr<ISceneCtor>&& c){
	table_.emplace(name, std::move(c));
}
std::shared_ptr<SceneObject> SceneObjectRegistry::Create(std::string_view name) const{
	auto it = table_.find(std::string(name));
	if (it == table_.end())
		throw std::runtime_error("Unknown SceneObject type: " + std::string(name));
	return it->second->New();
}
std::vector<std::string> SceneObjectRegistry::ListTypes() const{
	std::vector<std::string> out;
	for (auto& [k, _] : table_) out.push_back(k);
	return out;
}
