#pragma once
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>


#include <Game/Battle/Movement/Formation/EnemyFormationController.h>

/*-----------------------------------------------------------------------------------------
 * EnemySpawnerConfig
 * - 敵スポナー設定構造体
 * - 敵のスポーン間隔・最大数・アクティブ範囲などのパラメータを管理
 *---------------------------------------------------------------------------------------*/
struct EnemySpawnerConfig : public SceneObjectConfig {
	float  spawnInterval = 1.0f;
	size_t maxSpawnCount = 5;

	float activateRadius   = 80.0f;	 // これ以内でスポーン許可/維持
	float deactivateRadius = 100.0f; // これを超えたらスポーン停止＆遠距離デスポーン
	bool  useXZDistance	   = true;	 // レール系はXZ距離でOK

	float shootStaggerOffset = 0.3f; // 射撃タイミングのずらしオフセット

	EnemyFormationConfig formation;
};

// ------------ JSON 連携 ------------
inline void to_json(nlohmann::json& j, const EnemySpawnerConfig& c) {
	j = nlohmann::json{
		{"name", c.name},
		{"spawnInterval", c.spawnInterval},
		{"maxSpawnCount", c.maxSpawnCount},
		{"activateRadius", c.activateRadius},
		{"deactivateRadius", c.deactivateRadius},
		{"useXZDistance", c.useXZDistance},
		{"transform", c.transform},
		{"shootStaggerOffset", c.shootStaggerOffset},

		{"formation", c.formation}};
}

inline void from_json(const nlohmann::json& j, EnemySpawnerConfig& c) {
	if(j.contains("name")) j.at("name").get_to(c.name);
	if(j.contains("spawnInterval")) j.at("spawnInterval").get_to(c.spawnInterval);
	if(j.contains("maxSpawnCount")) j.at("maxSpawnCount").get_to(c.maxSpawnCount);
	if(j.contains("activateRadius")) j.at("activateRadius").get_to(c.activateRadius);
	if(j.contains("deactivateRadius")) j.at("deactivateRadius").get_to(c.deactivateRadius);
	if(j.contains("useXZDistance")) j.at("useXZDistance").get_to(c.useXZDistance);
	if(j.contains("transform")) j.at("transform").get_to(c.transform);
	if(j.contains("shootStaggerOffset")) j.at("shootStaggerOffset").get_to(c.shootStaggerOffset);

	if(j.contains("formation")) j.at("formation").get_to(c.formation);
}