#pragma once

// engine
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/System/Event/EventBus.h>

// std
#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

class SceneObject;

/*-----------------------------------------------------------------------------------------
 * SceneObjectLibrary
 * - シーンオブジェクト管理クラス
 * - シーン上の全オブジェクトの登録・削除・検索・一覧取得を担当
 *---------------------------------------------------------------------------------------*/
class SceneObjectLibrary {
public:
    //====================================================================*//
    //      public functions
    //====================================================================*//
    SceneObjectLibrary();
    ~SceneObjectLibrary();

    /**
     * @brief オブジェクト追加
     * @param object
     */
    void AddObject(const std::shared_ptr<SceneObject>& object);

    /**
     * @brief オブジェクトの削除
     * @param object
     * @return
     */
    bool RemoveObject(const std::shared_ptr<SceneObject>& object);

    /**
     * @brief オブジェクトの削除（ID指定）
     * @param id
     * @return
     */
    bool RemoveObject(Guid id);

    /**
     * @brief 全オブジェクト削除
     */
    void Clear();

    /**
     * @brief idからオブジェクトを検索
     * @param id
     * @return 検索結果のオブジェクト（見つからなければ nullptr）
     */
    std::shared_ptr<SceneObject> Find(Guid id) const;

    /**
     * @brief 名前からオブジェクトを検索（最初に見つかったものを返す）
     * @param name
     * @return 検索結果のオブジェクト（見つからなければ nullptr）
     */
    std::shared_ptr<SceneObject> FindByName(const std::string& name) const;
    
    std::shared_ptr<SceneObject> FindSharedByPickingID(uint32_t pickingID) const;

    /**
     * @brief 型からオブジェクトを検索
     * @tparam TObject
     * @return 検索結果の配列
     */
    template <class TObject>
    std::vector<std::shared_ptr<TObject>> FindByType() const;

    /**
     * @brief 全オブジェクト取得（生ポインタ版）
     * @return シーン上のオブジェクトリストの配列
     */
    std::vector<SceneObject*> GetAllObjectsRaw() const;

    /**
     * @brief 全オブジェクト取得（shared_ptr版）
     * @return シーン上のオブジェクトリストの配列
     */
    std::vector<std::shared_ptr<SceneObject>> GetAllObjectsShared() const;

    /**
     * @brief オブジェクトが含まれているか
     * @param obj
     * @return
     */
    bool Contains(const std::shared_ptr<SceneObject>& obj) const;

    /**
     * @brief 指定IDのオブジェクトが含まれているか
     * @param id
     * @return
     */
    bool Contains(Guid id) const { return objects_.contains(id); }

	/**
	 * \brief オブジェクトマップへの直接アクセス（読み取り専用）
	 * \return オブジェクトマップ
	 */
	const std::unordered_map<Guid, std::shared_ptr<SceneObject>>& GetObjects() const { return objects_; }

private:
    //====================================================================*//
    //      private variables
    //====================================================================*//
    std::unordered_map<Guid, std::shared_ptr<SceneObject>> objects_;
    std::unordered_map<std::string, uint32_t>              nameCounters_;
	EventBus::Connection connDestroy_;

    static uint32_t nextPickingID_;
};

// =====================================================
// テンプレート実装
// =====================================================
template <class TObject>
std::vector<std::shared_ptr<TObject>> SceneObjectLibrary::FindByType() const {
    static_assert(std::is_base_of_v<SceneObject, TObject>,
                  "TObject must derive from SceneObject");

    std::vector<std::shared_ptr<TObject>> result;
    result.reserve(objects_.size());

    for (const auto& [id, sp] : objects_) {
        if (!sp) continue;
        if (auto casted = std::dynamic_pointer_cast<TObject>(sp)) {
            result.emplace_back(std::move(casted));
        }
    }
    return result;
}
