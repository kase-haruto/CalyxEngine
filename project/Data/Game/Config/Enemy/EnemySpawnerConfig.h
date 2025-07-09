#pragma once

#include <string>
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

struct EnemySpawnerConfig {
	std::string name = "EnemySpawner";

	float rotationSpeed = 1.0f;   //< 回転速度
	Vector3 rotationDir = { 0.0f, 1.0f, 0.0f }; //< 回転軸
	float spawnInterval = 5.0f;   //< スポーン間隔
	Vector3 spawnAreaMin = { -10.0f, 0.0f, -30.0f }; //< スポーン範囲最小
	Vector3 spawnAreaMax = { 10.0f, 5.0f, -30.0f };  //< スポーン範囲最大
	size_t maxSpawnCount = 5;     //< 最大同時スポーン数
};

// ----------------------------------------------------------
// JSON 連携
// ----------------------------------------------------------
inline void to_json(nlohmann::json& j, const EnemySpawnerConfig& config) {
	j = nlohmann::json{
		{"name", config.name},
		{"rotationSpeed", config.rotationSpeed},
		{"rotationDir", config.rotationDir},
		{"spawnInterval", config.spawnInterval},
		{"spawnAreaMin", config.spawnAreaMin},
		{"spawnAreaMax", config.spawnAreaMax},
		{"maxSpawnCount", config.maxSpawnCount},
	};
}

inline void from_json(const nlohmann::json& j, EnemySpawnerConfig& config) {
	j.at("name").get_to(config.name);
	j.at("rotationSpeed").get_to(config.rotationSpeed);
	j.at("rotationDir").get_to(config.rotationDir);
	j.at("spawnInterval").get_to(config.spawnInterval);
	j.at("spawnAreaMin").get_to(config.spawnAreaMin);
	j.at("spawnAreaMax").get_to(config.spawnAreaMax);
	j.at("maxSpawnCount").get_to(config.maxSpawnCount);
}
