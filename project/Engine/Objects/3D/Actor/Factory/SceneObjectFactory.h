#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>


#include <externals/nlohmann/json.hpp>

#include <memory>
#include <unordered_map>
#include <functional>

/* ========================================================================
/*		シーンオブジェクト作成
/* ===================================================================== */
class ObjectFactory {
public:
	using CreatorFunc = std::function<std::unique_ptr<SceneObject>(const nlohmann::json&)>;

	// オブジェクトの生成
	static std::unique_ptr<SceneObject> Create(ObjectType type, const nlohmann::json& j);

private:
	static std::unordered_map<ObjectType, CreatorFunc> registry_;
};
