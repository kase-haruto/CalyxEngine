#pragma once

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <vector>

/**
 * オブジェクト管理
 */
class SceneObjectManager {
public:
	SceneObjectManager(); // コンストラクタ
	~SceneObjectManager()                                    = default;
	SceneObjectManager(const SceneObjectManager&)            = delete;
	SceneObjectManager& operator=(const SceneObjectManager&) = delete;
	SceneObjectManager(SceneObjectManager&&)                 = delete;
	SceneObjectManager& operator=(SceneObjectManager&&)      = delete;

public:
	//==================================================================*//
	//			public functions
	//==================================================================*//
	/**
	 * \brief すべてのオブジェクトを削除
	 */
	void ClearAllObject();
	/**
	 * \brief gameObjectのみを削除
	 */
	void ClearGameObjects();
	/**
	 * \brief 終了処理
	 */
	void Finalize();

private:
	//==================================================================*//
	//			private variables
	//==================================================================*//
	std::vector<SceneObject*> allSceneObjects_; // 全てのオブジェクト
};