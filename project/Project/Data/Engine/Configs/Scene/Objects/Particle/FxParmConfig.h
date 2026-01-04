#pragma once

#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp> // 必須

// -------------------------
// FxValueMode enum
// -------------------------
enum class FxValueMode{
	Constant,
	Random,
	RandomSphere
};

// JSON enum対応
inline void to_json(nlohmann::json& j, const FxValueMode& mode){
	j = static_cast< int >(mode);
}
inline void from_json(const nlohmann::json& j, FxValueMode& mode){
	mode = static_cast< FxValueMode >(j.get<int>());
}

// -------------------------
// テンプレート基底
// -------------------------
template<typename T>
struct FxParamConfig{
	FxValueMode mode = FxValueMode::Constant;
	T constant {};
	T min {};
	T max {};
};

// -------------------------
// 型別特殊構造体
// -------------------------
struct FxFloatParamConfig : public FxParamConfig<float>{
	using FxParamConfig<float>::FxParamConfig;
	FxFloatParamConfig() = default;
	FxFloatParamConfig(const FxParamConfig<float>& base)
		: FxParamConfig<float>(base){}
};

struct FxVector3ParamConfig : public FxParamConfig<Vector3>{
	using FxParamConfig<Vector3>::FxParamConfig;
	FxVector3ParamConfig() = default;
	FxVector3ParamConfig(const FxParamConfig<Vector3>& base)
		: FxParamConfig<Vector3>(base){}
};

// -------------------------
// JSON対応
// -------------------------
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FxFloatParamConfig,
								   mode,
								   constant,
								   min,
								   max)

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(FxVector3ParamConfig,
									   mode,
									   constant,
									   min,
									   max)
