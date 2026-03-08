#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Foundation/Math/Vector3.h>

struct WorldTransformConfig final {
	//========================= variable =========================
	CalyxMath::Vector3	  translation = {0.0f, 0.0f, 0.0f};					   //< 位置
	CalyxMath::Quaternion rotation	  = CalyxMath::Quaternion::MakeIdentity(); //< 回転
	CalyxMath::Vector3	  scale		  = {1.0f, 1.0f, 1.0f};					   //< スケール

	bool inheritTranslate = true;
	bool inheritRotate	  = true;
	bool inheritScale	  = true;
};

inline void to_json(nlohmann::json& j, const WorldTransformConfig& config) {
	j = nlohmann::json{
		{"translation", config.translation},
		{"rotation", config.rotation},
		{"scale", config.scale},
		{"inheritTranslate", config.inheritTranslate},
		{"inheritRotate", config.inheritRotate},
		{"inheritScale", config.inheritScale}};
}

inline void from_json(const nlohmann::json& j, WorldTransformConfig& config) {
	config.translation = j.value("translation", CalyxMath::Vector3{0.0f, 0.0f, 0.0f});
	config.rotation	   = j.value("rotation", CalyxMath::Quaternion::MakeIdentity());
	config.scale	   = j.value("scale", CalyxMath::Vector3{1.0f, 1.0f, 1.0f});

	config.inheritTranslate = j.value("inheritTranslate", true);
	config.inheritRotate	= j.value("inheritRotate", true);
	config.inheritScale		= j.value("inheritScale", true);
}