#include "SceneControllerFactory.h"


SceneControllerFactory& SceneControllerFactory::Get(){
	static SceneControllerFactory inst; return inst;
}

void SceneControllerFactory::Register(const std::string& name, Creator c){ map_[name] = std::move(c); }

std::unique_ptr<SceneController> SceneControllerFactory::Create(const std::string& name) const{
	if (auto it = map_.find(name); it != map_.end()) return it->second();
	return nullptr;
}