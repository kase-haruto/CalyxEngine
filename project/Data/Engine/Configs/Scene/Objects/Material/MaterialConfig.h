#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector4.h>
#include <externals/nlohmann/json.hpp>
#include <cstdint>
#include <string>
struct MaterialConfig final{
	 //========================= variable =========================
	CalyxEngine::Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };	//< 色
	int32_t enableLighting = 0;					//< ライティング有無
	float shininess = 20.0f;					//< 光沢
	float enviromentCoefficient = 0.5f;			//< 環境光の強さ
	bool isReflect = false;						//< 反射
	float roughness = 0.5f;						//< 反射の粗さ
	int currentLightingMode_ = 0;				//< ライティングモード
	CalyxEngine::Vector4 toonHighlightColor = { 1.15f, 1.10f, 1.00f, 1.0f };
	CalyxEngine::Vector4 toonBaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	CalyxEngine::Vector4 toonMidShadowColor = { 0.72f, 0.76f, 0.86f, 1.0f };
	CalyxEngine::Vector4 toonShadowColor = { 0.42f, 0.46f, 0.58f, 1.0f };
	float toonBaseStep = 0.25f;
	float toonBaseFeather = 0.03f;
	float toonShadeStep = -0.15f;
	float toonShadeFeather = 0.03f;
	float toonThreshold1 = -0.15f;
	float toonThreshold2 = 0.25f;
	float toonThreshold3 = 0.82f;
	float toonEdgeSoftness = 0.03f;
	float toonSpecularThreshold = 0.96f;
	float toonSpecularSoftness = 0.02f;
	float toonSpecularIntensity = 0.35f;
};

inline void to_json(nlohmann::json& j, const MaterialConfig& c) {
	j = nlohmann::json{
		{"color", c.color},
		{"enableLighting", c.enableLighting},
		{"shininess", c.shininess},
		{"enviromentCoefficient", c.enviromentCoefficient},
		{"isReflect", c.isReflect},
		{"roughness", c.roughness},
		{"currentLightingMode_", c.currentLightingMode_},
		{"toonHighlightColor", c.toonHighlightColor},
		{"toonBaseColor", c.toonBaseColor},
		{"toonMidShadowColor", c.toonMidShadowColor},
		{"toonShadowColor", c.toonShadowColor},
		{"toonBaseStep", c.toonBaseStep},
		{"toonBaseFeather", c.toonBaseFeather},
		{"toonShadeStep", c.toonShadeStep},
		{"toonShadeFeather", c.toonShadeFeather},
		{"toonThreshold1", c.toonThreshold1},
		{"toonThreshold2", c.toonThreshold2},
		{"toonThreshold3", c.toonThreshold3},
		{"toonEdgeSoftness", c.toonEdgeSoftness},
		{"toonSpecularThreshold", c.toonSpecularThreshold},
		{"toonSpecularSoftness", c.toonSpecularSoftness},
		{"toonSpecularIntensity", c.toonSpecularIntensity}
	};
}

inline void from_json(const nlohmann::json& j, MaterialConfig& c) {
	if(j.contains("color")) j.at("color").get_to(c.color);
	if(j.contains("enableLighting")) j.at("enableLighting").get_to(c.enableLighting);
	if(j.contains("shininess")) j.at("shininess").get_to(c.shininess);
	if(j.contains("enviromentCoefficient")) j.at("enviromentCoefficient").get_to(c.enviromentCoefficient);
	if(j.contains("isReflect")) j.at("isReflect").get_to(c.isReflect);
	if(j.contains("roughness")) j.at("roughness").get_to(c.roughness);
	if(j.contains("currentLightingMode_")) j.at("currentLightingMode_").get_to(c.currentLightingMode_);
	if(j.contains("toonHighlightColor")) j.at("toonHighlightColor").get_to(c.toonHighlightColor);
	if(j.contains("toonBaseColor")) j.at("toonBaseColor").get_to(c.toonBaseColor);
	if(j.contains("toonMidShadowColor")) j.at("toonMidShadowColor").get_to(c.toonMidShadowColor);
	if(j.contains("toonShadowColor")) j.at("toonShadowColor").get_to(c.toonShadowColor);
	if(j.contains("toonThreshold1")) j.at("toonThreshold1").get_to(c.toonThreshold1);
	if(j.contains("toonThreshold2")) j.at("toonThreshold2").get_to(c.toonThreshold2);
	if(j.contains("toonThreshold3")) j.at("toonThreshold3").get_to(c.toonThreshold3);
	if(j.contains("toonEdgeSoftness")) j.at("toonEdgeSoftness").get_to(c.toonEdgeSoftness);
	if(j.contains("toonBaseStep")) j.at("toonBaseStep").get_to(c.toonBaseStep);
	else c.toonBaseStep = c.toonThreshold2;
	if(j.contains("toonBaseFeather")) j.at("toonBaseFeather").get_to(c.toonBaseFeather);
	else c.toonBaseFeather = c.toonEdgeSoftness;
	if(j.contains("toonShadeStep")) j.at("toonShadeStep").get_to(c.toonShadeStep);
	else c.toonShadeStep = c.toonThreshold1;
	if(j.contains("toonShadeFeather")) j.at("toonShadeFeather").get_to(c.toonShadeFeather);
	else c.toonShadeFeather = c.toonEdgeSoftness;
	if(j.contains("toonSpecularThreshold")) j.at("toonSpecularThreshold").get_to(c.toonSpecularThreshold);
	if(j.contains("toonSpecularSoftness")) j.at("toonSpecularSoftness").get_to(c.toonSpecularSoftness);
	if(j.contains("toonSpecularIntensity")) j.at("toonSpecularIntensity").get_to(c.toonSpecularIntensity);
}
