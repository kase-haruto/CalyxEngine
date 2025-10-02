#pragma once

enum class LightType{
	Directional,	// ディレクショナル
	Point,			// ポイント
};

// ライティングモードの定義
enum LightingMode{
	HalfLambert = 0,
	Lambert,
	specularReflection,
	NoLighting,
	UnlitColor
};