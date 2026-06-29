#pragma once

#include <Engine/Collision/CollisionMatrix.h>
#include <externals/nlohmann/json.hpp>

#include <string>
#include <vector>

struct CollisionLayer {
	// シーンやPrefabに保存される安定した参照値。
	CollisionLayerId id = kDefaultCollisionLayerId;
	// InspectorやCollision Settingsにだけ使用する表示名。
	std::string name;
};

/*-----------------------------------------------------------------------------------------
 * CollisionLayerSettings
 * - 1シーンで利用可能なLayerのID、表示名、CollisionMatrixを一元管理する。
 * - SceneContextが所有し、Colliderは名前を保存せず、この設定が名前変更や削除を吸収する。
 *---------------------------------------------------------------------------------------*/
class CollisionLayerSettings {
public:
	CollisionLayerSettings();

	// 現在のSceneContextが所有する設定を、既存Collider/UIから参照するための窓口。
	// Sceneが存在しない場合は内部のDefault設定を返す。
	static CollisionLayerSettings* GetInstance();
	static void SetActiveSettings(CollisionLayerSettings* settings);

	CollisionLayerSettings(const CollisionLayerSettings&) = delete;
	CollisionLayerSettings& operator=(const CollisionLayerSettings&) = delete;

	// 未使用の最小IDを割り当てる。空名、重複名、上限超過ではfalseを返す。
	bool AddLayer(const std::string& name, CollisionLayerId* outLayerId = nullptr);

	// Default以外のLayerを削除し、対応するMatrixの行・列も無効化する。
	bool RemoveLayer(CollisionLayerId layerId);

	// IDを維持したまま表示名だけを変更する。
	bool RenameLayer(CollisionLayerId layerId, const std::string& newName);

	// UIで安定した順序に表示できるよう、一覧はID昇順で保持する。
	const std::vector<CollisionLayer>& GetLayers() const { return layers_; }

	// 未登録IDでは参照切れを明示する固定文字列を返す。
	const std::string& GetLayerName(CollisionLayerId layerId) const;

	// 数値範囲だけでなく、現在Layer一覧に登録されていることまで確認する。
	bool IsValidLayerId(CollisionLayerId layerId) const;

	// SceneSerializerから呼ばれるシーン単位の保存・復元処理。
	nlohmann::json ToJson() const;
	void ApplyJson(const nlohmann::json& json);
	void ResetToDefault();

	// Matrix編集UIとCollisionManagerが同一の設定を参照するためのアクセサ。
	CollisionMatrix& GetMatrix() { return matrix_; }
	const CollisionMatrix& GetMatrix() const { return matrix_; }

private:
	bool IsLayerNameAvailable(const std::string& name, CollisionLayerId ignoredLayerId) const;
	static CollisionLayerSettings* activeSettings_;

	std::vector<CollisionLayer> layers_;
	CollisionMatrix matrix_;
};
