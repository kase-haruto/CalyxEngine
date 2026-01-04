#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */

// engine
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/FxParmConfig.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

#include <type_traits>

#include <externals/imgui/imgui.h>

template<typename T>
class FxParam{
public:
	FxParam() = default;
	void SetConstant(const T& value);
	void SetRandom(const T& min, const T& max);
	T Get() const;
	FxValueMode GetMode() const{ return mode_; }
	const T& GetMin() const{ return min_; }
	const T& GetMax() const{ return max_; }
	const T& GetConstant() const{ return constant_; }

	//config
	void FromConfig(const FxParamConfig<T>& cfg);
	FxParamConfig<T> ToConfig() const;

public:
	//===================================================================*/
	//					static methods
	//===================================================================*/
	// 定数値で初期化
	static FxParam<T> MakeConstant(const T& value = getDefault()){
		FxParam<T> param;
		param.mode_ = FxValueMode::Constant;
		param.constant_ = value;
		return param;
	}

	// ランダム値で初期化
	static FxParam<T> MakeRandom(const T& min = getHalf(getDefault()), const T& max = getDefault()){
		FxParam<T> param;
		param.mode_ = FxValueMode::Random;
		param.min_ = min;
		param.max_ = max;
		return param;
	}

private:
	static T getDefault(){
		if constexpr (std::is_same_v<T, Vector3>){
			return Vector3(1.0f, 1.0f, 1.0f);
		} else if constexpr (std::is_arithmetic_v<T>){
			return T(1);
		} else{
			return T {};
		}
	}
	static T getHalf(const T& v){
		if constexpr (std::is_same_v<T, Vector3>){
			return v * 0.5f;
		} else if constexpr (std::is_arithmetic_v<T>){
			return v * T(0.5f);
		} else{
			return T {};
		}
	}

private:
	FxValueMode mode_ = FxValueMode::Constant;
	T constant_ {};	//< 定数値
	T min_ {};		//< 最小値（ランダムモード用）
	T max_ {};		//< 最大値（ランダムモード用）
};


/////////////////////////////////////////////////////////////////////////////////////////
//		定数の設定
/////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline void FxParam<T>::SetConstant(const T& value){
	mode_ = FxValueMode::Constant;
	constant_ = value;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ランダム数の設定
/////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline void FxParam<T>::SetRandom(const T& min, const T& max){
	mode_ = FxValueMode::Random;
	min_ = min;
	max_ = max;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		値の取得（スカラー型用）
/////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
inline T FxParam<T>::Get() const{
	if (mode_ == FxValueMode::Constant){
		return constant_;
	} else{
		return Random::Generate<T>(min_, max_);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		値の取得（Vector3 専用特殊化）
/////////////////////////////////////////////////////////////////////////////////////////
template<>
inline Vector3 FxParam<Vector3>::Get() const {
	switch (mode_) {
		case FxValueMode::Constant:
			return constant_;
		case FxValueMode::Random: // 立方体ランダム
			return Random::GenerateVector3(min_, max_);
		case FxValueMode::RandomSphere:
			{ // 球状ランダム
				Vector3 center = (min_ + max_) * 0.5f;
				float radius = ((max_ - min_).Length()) * 0.5f;
				Vector3 dir = Random::GenerateUnitVector3();
				float dist = Random::Generate<float>(0.0f, radius);
				return center + dir * dist;
			}
		default:
			return constant_;
	}
}

//===================================================================*/
//		Config からの設定
//===================================================================*/
template<typename T>
inline void FxParam<T>::FromConfig(const FxParamConfig<T>& cfg){
	mode_ = cfg.mode;
	constant_ = cfg.constant;
	min_ = cfg.min;
	max_ = cfg.max;
}

//===================================================================*/
//		Config への変換
//===================================================================*/
template<typename T>
inline FxParamConfig<T> FxParam<T>::ToConfig() const{
	return FxParamConfig<T>{ mode_, constant_, min_, max_ };
}

namespace ImGuiHelpers{

	inline bool DrawFxParamGui(const char* label, FxParam<float>& param) {
		bool changed = false;

		bool isRandom = (param.GetMode() == FxValueMode::Random);
		bool useRandom = isRandom;

		ImGui::PushID(label);

		if (ImGui::Checkbox("##UseRandom", &useRandom)) {
			if (useRandom != isRandom) {
				if (useRandom) {
					param.SetRandom(param.GetMin(), param.GetMax());
				} else {
					param.SetConstant(param.GetConstant());
				}
				changed = true;
			}
		}
		ImGui::SameLine();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		bool open = ImGui::CollapsingHeader(label, flags);

		if (open) {
			ImGui::Indent();
			if (!useRandom) {
				float value = param.GetConstant();
				if (GuiCmd::DragFloat("Constant", value)) {
					param.SetConstant(value);
					changed = true;
				}
			} else {
				float minVal = param.GetMin();
				float maxVal = param.GetMax();

				bool edited = false;
				if (GuiCmd::DragFloat("Min", minVal)) edited = true;
				if (GuiCmd::DragFloat("Max", maxVal)) edited = true;

				if (edited) {
					if (minVal > maxVal) std::swap(minVal, maxVal);
					param.SetRandom(minVal, maxVal);
					changed = true;
				}
			}
			ImGui::Unindent();
		}

		ImGui::PopID();
		return changed;
	}

	inline bool DrawFxParamGui(const char* label, FxParam<Vector3>& param) {
		bool changed = false;

		bool isRandom = (param.GetMode() == FxValueMode::Random);
		bool useRandom = isRandom;

		ImGui::PushID(label);

		if (ImGui::Checkbox("##UseRandom", &useRandom)) {
			if (useRandom != isRandom) {
				if (useRandom) {
					param.SetRandom(param.GetMin(), param.GetMax());
				} else {
					param.SetConstant(param.GetConstant());
				}
				changed = true;
			}
		}
		ImGui::SameLine();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
		bool open = ImGui::CollapsingHeader(label, flags);

		if (open) {
			ImGui::Indent();
			if (!useRandom) {
				Vector3 value = param.GetConstant();
				if (GuiCmd::DragFloat3("Constant", value)) {
					param.SetConstant(value);
					changed = true;
				}
			} else {
				Vector3 minVal = param.GetMin();
				Vector3 maxVal = param.GetMax();

				bool edited = false;
				if (GuiCmd::DragFloat3("Min", minVal)) edited = true;
				if (GuiCmd::DragFloat3("Max", maxVal)) edited = true;

				if (edited) {
					// 各成分で min > max をチェックして入れ替え
					if (minVal.x > maxVal.x) std::swap(minVal.x, maxVal.x);
					if (minVal.y > maxVal.y) std::swap(minVal.y, maxVal.y);
					if (minVal.z > maxVal.z) std::swap(minVal.z, maxVal.z);

					param.SetRandom(minVal, maxVal);
					changed = true;
				}
			}
			ImGui::Unindent();
		}

		ImGui::PopID();
		return changed;
	}


} // namespace ImGuiHelpers