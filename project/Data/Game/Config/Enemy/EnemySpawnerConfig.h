#pragma once

#include <string>
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

struct EnemySpawnerConfig
		: public SceneObjectConfig{
	float rotationSpeed = 1.0f; //< 回転速度
	Vector3 rotationDir = {0.0f, 1.0f, 0.0f}; //< 回転軸
	float spawnInterval = 5.0f; //< スポーン間隔
	Vector3 spawnAreaMin = {-10.0f, 0.0f, -30.0f}; //< スポーン範囲最小
	Vector3 spawnAreaMax = {10.0f, 5.0f, -30.0f}; //< スポーン範囲最大
	size_t maxSpawnCount = 5; //< 最大同時スポーン数
};

// ----------------------------------------------------------
// JSON 連携
// ----------------------------------------------------------
inline void to_json(nlohmann::json& j, const EnemySpawnerConfig& config){
	j = nlohmann::json{
				{"name", config.name},
				{"rotationSpeed", config.rotationSpeed},
				{"rotationDir", config.rotationDir},
				{"spawnInterval", config.spawnInterval},
				{"spawnAreaMin", config.spawnAreaMin},
				{"spawnAreaMax", config.spawnAreaMax},
				{"maxSpawnCount", config.maxSpawnCount},
				{"transform", config.transform},
			};
}

inline void from_json(const nlohmann::json& j, EnemySpawnerConfig& config){
	// あるキーだけ上書き（無ければ既存値を保持）
	if (j.contains("name")) j.at("name").get_to(config.name);
	if (j.contains("rotationSpeed")) j.at("rotationSpeed").get_to(config.rotationSpeed);
	if (j.contains("rotationDir")) j.at("rotationDir").get_to(config.rotationDir);
	if (j.contains("spawnInterval")) j.at("spawnInterval").get_to(config.spawnInterval);
	if (j.contains("spawnAreaMin")) j.at("spawnAreaMin").get_to(config.spawnAreaMin);
	if (j.contains("spawnAreaMax")) j.at("spawnAreaMax").get_to(config.spawnAreaMax);
	if (j.contains("maxSpawnCount")) j.at("maxSpawnCount").get_to(config.maxSpawnCount);
	if (j.contains("transform")) j.at("transform").get_to(config.transform);
}
