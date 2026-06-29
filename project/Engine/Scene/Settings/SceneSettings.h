#pragma once

#include <Engine/Collision/CollisionLayerSettings.h>
#include <externals/nlohmann/json.hpp>

/*-----------------------------------------------------------------------------------------
 * SceneSettings
 * - SceneObjectではなくシーン全体に作用する設定を所有するルートコンテナ。
 * - 今後Rendering、Audio、Navigation等を追加する場合も、このクラスへカテゴリを追加する。
 *---------------------------------------------------------------------------------------*/
class SceneSettings {
public:
	CollisionLayerSettings& GetCollisionSettings() { return collisionSettings_; }
	const CollisionLayerSettings& GetCollisionSettings() const { return collisionSettings_; }

	// SceneSerializerがシーンJSONのsettingsセクションを保存・復元するための境界。
	nlohmann::json ToJson() const;
	void ApplyJson(const nlohmann::json& json);
	void ResetToDefault();

private:
	// Collision設定はColliderインスタンスではなく、同じシーン内で共有されるルールとして所有する。
	CollisionLayerSettings collisionSettings_;
};
