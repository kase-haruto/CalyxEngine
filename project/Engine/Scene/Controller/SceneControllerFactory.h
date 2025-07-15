#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Scene/Controller/SceneController.h>

//c++
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class SceneControllerFactory{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	using Creator = std::function<std::unique_ptr<SceneController>()>;

	static SceneControllerFactory& Get();
	void Register(const std::string& name, Creator c);
	std::unique_ptr<SceneController> Create(const std::string& name) const;

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	std::unordered_map<std::string, Creator> map_;
};

// 登録マクロ
#define REGISTER_SCENE_CONTROLLER(T) \
    namespace{ const bool _rg_##T = []{ \
        SceneControllerFactory::Get().Register(#T, []{ return std::make_unique<T>(); }); \
        return true;}(); }