#pragma once

#include <externals/nlohmann/json.hpp>

/*-----------------------------------------------------------------------------------------
 * PhysicsBodyConfig
 * - 物理応答用の保存設定
 * - コライダーの検出設定とは分け、押し戻し対象かどうかを明示的に管理する
 *---------------------------------------------------------------------------------------*/
struct PhysicsBodyConfig {
	bool enabled = true;		 //< 物理応答を行うか
	int	 bodyType = 0;			 //< 0:Static, 1:Kinematic
	float pushbackRatio = 1.0f; //< 押し戻し量の反映率
};

inline void to_json(nlohmann::json& j, const PhysicsBodyConfig& c) {
	j = nlohmann::json{
		{"enabled", c.enabled},
		{"bodyType", c.bodyType},
		{"pushbackRatio", c.pushbackRatio}};
}

inline void from_json(const nlohmann::json& j, PhysicsBodyConfig& c) {
	c.enabled = j.value("enabled", true);
	c.bodyType = j.value("bodyType", 0);
	c.pushbackRatio = j.value("pushbackRatio", 1.0f);
}
