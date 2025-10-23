#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class SceneObject;

/* ========================================================================
/*		sceneObjectCtor
/* ===================================================================== */
class ISceneCtor{
public:
	virtual std::shared_ptr<SceneObject> New() const = 0;
	virtual ~ISceneCtor() = default;
};

template<class T>
class SceneCtor final 
	: public ISceneCtor{
public:
	std::shared_ptr<SceneObject> New() const override{
		return std::make_shared<T>();
	}
};


/* ========================================================================
/*		jsonの文字列からインスタンスを作成するため
/* ===================================================================== */
class SceneObjectRegistry{
public:
	static SceneObjectRegistry& Get();

	/// <summary>
	/// 文字列名とそれを生成するctorを保存
	/// </summary>
	/// <param name="typeName"></param>
	/// <param name="ctor"></param>
	void Register(std::string_view typeName, std::unique_ptr<ISceneCtor>&& ctor);

	/// <summary>
	/// 登録済みの名前に対応するオブジェクトを生成
	/// </summary>
	/// <param name="typeName"></param>
	/// <returns></returns>
	std::shared_ptr<SceneObject> Create(std::string_view typeName) const;

	/// <summary>
	/// 登録済みオブジェクト名を一覧で返す
	/// </summary>
	/// <returns></returns>
	std::vector<std::string> ListTypes() const;

private:
	/// <summary>
	/// オブジェクト登録テーブル
	/// </summary>
	std::unordered_map<std::string, std::unique_ptr<ISceneCtor>> table_;
};

// 登録マクロ
#define REGISTER_SCENE_OBJECT(T) \
	namespace { const bool _rg_##T = []{ \
		SceneObjectRegistry::Get().Register(#T, std::make_unique<SceneCtor<T>>()); \
		return true; }(); }
