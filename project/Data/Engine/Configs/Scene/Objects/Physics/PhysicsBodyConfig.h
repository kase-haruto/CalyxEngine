#pragma once

#include <externals/nlohmann/json.hpp>

/*-----------------------------------------------------------------------------------------
 * PhysicsBodyConfig
 * - 物理応答用の保存設定
 * - コライダーの検出設定とは分け、押し戻し対象かどうかを明示的に管理する
 *---------------------------------------------------------------------------------------*/
struct PhysicsBodyConfig {
	bool enabled = true;		 //< 物理応答を行うか
	int	 bodyType = 0;			 //< 0:Static, 1:Kinematic, 2:Dynamic
	float pushbackRatio = 1.0f; //< 押し戻し量の反映率
	bool useGravity = true;      //< Dynamicへワールド重力を適用するか
	float gravityScale = 1.0f;  //< ワールド重力へ掛ける倍率
	float mass = 1.0f;          //< Dynamicの質量
};

inline void to_json(nlohmann::json& j, const PhysicsBodyConfig& c) {
	j = nlohmann::json{
		{"enabled", c.enabled},
		{"bodyType", c.bodyType},
		{"pushbackRatio", c.pushbackRatio},
		{"useGravity", c.useGravity},
		{"gravityScale", c.gravityScale},
		{"mass", c.mass}};
}

inline void from_json(const nlohmann::json& j, PhysicsBodyConfig& c) {
	c.enabled = j.value("enabled", true);
	c.bodyType = j.value("bodyType", 0);
	c.pushbackRatio = j.value("pushbackRatio", 1.0f);
	c.useGravity = j.value("useGravity", true);
	c.gravityScale = j.value("gravityScale", 1.0f);
	c.mass = j.value("mass", 1.0f);
}
