#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class SceneObject;

class ISceneCtor{
public:
	virtual std::shared_ptr<SceneObject> New() const = 0;
	virtual ~ISceneCtor() = default;
};

template<class T>
class SceneCtor final : public ISceneCtor{
public:
	std::shared_ptr<SceneObject> New() const override{
		return std::make_shared<T>();
	}
};

class SceneObjectRegistry{
public:
	static SceneObjectRegistry& Get();

	void Register(std::string_view typeName, std::unique_ptr<ISceneCtor>&& ctor);
	std::shared_ptr<SceneObject> Create(std::string_view typeName) const;
	std::vector<std::string>     ListTypes() const;

private:
	std::unordered_map<std::string, std::unique_ptr<ISceneCtor>> table_;
};

// 登録マクロ
#define REGISTER_SCENE_OBJECT(T) \
	namespace { const bool _rg_##T = []{ \
		SceneObjectRegistry::Get().Register(#T, std::make_unique<SceneCtor<T>>()); \
		return true; }(); }
