#pragma once
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>

struct EnemySpawnerConfig : public SceneObjectConfig {
	float rotationSpeed = 1.0f;
	Vector3 rotationDir = { 0.0f, 1.0f, 0.0f };
	float spawnInterval = 5.0f;
	Vector3 spawnAreaMin = { -10.0f, 0.0f, -30.0f };
	Vector3 spawnAreaMax = { 10.0f, 5.0f, -30.0f };
	size_t maxSpawnCount = 5;

	float activateRadius = 80.0f;       // これ以内でスポーン許可/維持
	float deactivateRadius = 100.0f;    // これを超えたらスポーン停止＆遠距離デスポーン
	bool  useXZDistance = true;         // レール系はXZ距離でOK
};

// ------------ JSON 連携 ------------
inline void to_json(nlohmann::json& j, const EnemySpawnerConfig& c) {
	j = nlohmann::json{
		{"name", c.name},
		{"rotationSpeed", c.rotationSpeed},
		{"rotationDir", c.rotationDir},
		{"spawnInterval", c.spawnInterval},
		{"spawnAreaMin", c.spawnAreaMin},
		{"spawnAreaMax", c.spawnAreaMax},
		{"maxSpawnCount", c.maxSpawnCount},
		{"activateRadius", c.activateRadius},
		{"deactivateRadius", c.deactivateRadius},
		{"useXZDistance", c.useXZDistance},
		{"transform", c.transform},
	};
}
inline void from_json(const nlohmann::json& j, EnemySpawnerConfig& c) {
	if (j.contains("name"))            j.at("name").get_to(c.name);
	if (j.contains("rotationSpeed"))   j.at("rotationSpeed").get_to(c.rotationSpeed);
	if (j.contains("rotationDir"))     j.at("rotationDir").get_to(c.rotationDir);
	if (j.contains("spawnInterval"))   j.at("spawnInterval").get_to(c.spawnInterval);
	if (j.contains("spawnAreaMin"))    j.at("spawnAreaMin").get_to(c.spawnAreaMin);
	if (j.contains("spawnAreaMax"))    j.at("spawnAreaMax").get_to(c.spawnAreaMax);
	if (j.contains("maxSpawnCount"))   j.at("maxSpawnCount").get_to(c.maxSpawnCount);
	if (j.contains("activateRadius"))  j.at("activateRadius").get_to(c.activateRadius);
	if (j.contains("deactivateRadius"))j.at("deactivateRadius").get_to(c.deactivateRadius);
	if (j.contains("useXZDistance"))   j.at("useXZDistance").get_to(c.useXZDistance);
	if (j.contains("transform"))       j.at("transform").get_to(c.transform);
}
