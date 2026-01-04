#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>

#include <vector>

class SceneObjectManager{
public:
	//==================================================================*//
	//			singleton
	//==================================================================*//
	SceneObjectManager(const SceneObjectManager&) = delete;
	SceneObjectManager& operator=(const SceneObjectManager&) = delete;
	SceneObjectManager(SceneObjectManager&&) = delete;
	SceneObjectManager& operator=(SceneObjectManager&&) = delete;
	static SceneObjectManager* GetInstance();

public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	void ClearAllObject();	//全てのオブジェクトを削除
	void ClearGameObjects(); // GameObjectのみを削除
	void Finalize();		//終了処理

	//objectの取得追加
	void AddObject(SceneObject* object);
	const std::vector<SceneObject*>& GetAllObjects()const{ return allSceneObjects_; }

	//objectの解放処理
	void RemoveObject(SceneObject* object);

private:
	//==================================================================*//
	//			private functions
	//==================================================================*//
	SceneObjectManager();				//コンストラクタ
	~SceneObjectManager() = default;	//デストラクタ

private:
	//==================================================================*//
	//			private variables
	//==================================================================*//
	std::vector<SceneObject*> allSceneObjects_;	//全てのオブジェクト
};

