#pragma once

#include <Engine/Collision/CollisionLayerSettings.h>
#include <Engine/Renderer/Sprite/SortingLayerSettings.h>
#include <externals/nlohmann/json.hpp>

/*-----------------------------------------------------------------------------------------
 * SceneSettings
 * - SceneObjectではなくシーン全体に作用する設定を所有するルートコンテナ。
 * - 今後Rendering、Audio、Navigation等を追加する場合も、このクラスへカテゴリを追加する。
 *---------------------------------------------------------------------------------------*/
/**
 * @brief SceneSettingsの機能を提供するクラスです。
 */
class SceneSettings {
public:
	CollisionLayerSettings& GetCollisionSettings() { return collisionSettings_; }
	const CollisionLayerSettings& GetCollisionSettings() const { return collisionSettings_; }
	SortingLayerSettings& GetSortingLayerSettings() { return sortingLayerSettings_; }
	const SortingLayerSettings& GetSortingLayerSettings() const { return sortingLayerSettings_; }

	// SceneSerializerがシーンJSONのsettingsセクションを保存・復元するための境界。
	nlohmann::json ToJson() const;
	void ApplyJson(const nlohmann::json& json);
	void ResetToDefault();

private:
	// Collision設定はColliderインスタンスではなく、同じシーン内で共有されるルールとして所有する。
	CollisionLayerSettings collisionSettings_;
	SortingLayerSettings sortingLayerSettings_;
};
