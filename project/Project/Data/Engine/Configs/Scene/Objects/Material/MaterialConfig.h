#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector4.h>
#include <cstdint>
#include <string>
struct MaterialConfig final{
	 //========================= variable =========================
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };	//< 色
	int32_t enableLighting = 0;					//< ライティング有無
	float shininess = 20.0f;					//< 光沢
	float enviromentCoefficient = 0.5f;			//< 環境光の強さ
	bool isReflect = false;						//< 反射
	int currentLightingMode_ = 0;				//< ライティングモード
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MaterialConfig,
								   color,
								   enableLighting,
								   shininess,
								   enviromentCoefficient,
								   isReflect,
								   currentLightingMode_)