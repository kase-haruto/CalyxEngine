#pragma once
#include <Data/Engine/Configs/Scene/Objects/Event/EventConfig.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

struct CameraTurnAroundEventConfig
	: public EventConfig {
	CalyxMath::Vector3 direction; // 振り向く方向
	float	time;	   // かかる時間
	int16_t easeType;  // イージングの処理
};

// ------------ JSON 連携 ------------
inline void to_json(nlohmann::json& j, const CameraTurnAroundEventConfig& v) {
	// まずは基底クラスの内容をシリアライズ
	to_json(j, static_cast<const EventConfig&>(v));

	// 派生クラスのパラメータを追加
	j["direction"] = v.direction;
	j["time"] = v.time;
	j["easeType"] = v.easeType;
}

inline void from_json(const nlohmann::json& j, CameraTurnAroundEventConfig& v) {
	// 基底クラスを復元
	from_json(j, static_cast<EventConfig&>(v));

	// 派生クラスの項目を復元
	j.at("direction").get_to(v.direction);
	j.at("time").get_to(v.time);
	j.at("easeType").get_to(v.easeType);
}