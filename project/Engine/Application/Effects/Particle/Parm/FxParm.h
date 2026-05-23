#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */

// engine
#include <Data/Engine/Configs/Scene/Objects/Particle/FxParmConfig.h>
#include <Engine/Foundation/Utility/Random/Random.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <type_traits>
#include <Engine/Application/Effects/FxGuiHelpers.h>
// external
#include <externals/imgui/imgui.h>

namespace CalyxEngine {
	/* ========================================================================
	/*		パーティクルパラメータ
	/* ===================================================================== */
	template <typename T>
	class FxParam {
	public:
		//===================================================================*/
		//					static methods
		//===================================================================*/
		FxParam();

		//--------- setter -----------------------------------------------------
		void SetConstant(const T& value);
		void SetRandom(const T& min, const T& max);

		//--------- getter ------------------------------------------------------
		T			Get() const;
		FxValueMode GetMode() const { return mode_; }
		const T&	GetMin() const { return min_; }
		const T&	GetMax() const { return max_; }
		const T&	GetConstant() const { return constant_; }

		//--------- config -------------------------------------------------------
		void			 FromConfig(const FxParamConfig<T>& cfg);
		FxParamConfig<T> ToConfig() const;

	public:
		//===================================================================*/
		//					static methods
		//===================================================================*/
		// 定数値で初期化
		static FxParam<T> MakeConstant(const T& value = getDefault()) {
			FxParam<T> param;
			param.mode_		= FxValueMode::Constant;
			param.constant_ = value;
			return param;
		}

		// ランダム値で初期化
		static FxParam<T> MakeRandom(const T& min = getHalf(getDefault()), const T& max = getDefault()) {
			FxParam<T> param;
			param.mode_ = FxValueMode::Random;
			param.min_	= min;
			param.max_	= max;
			return param;
		}

	private:
		static T getDefault() {
			if constexpr(std::is_same_v<T, CalyxEngine::Vector3>) {
				return CalyxEngine::Vector3(1.0f, 1.0f, 1.0f);
			} else if constexpr(std::is_arithmetic_v<T>) {
				return T(1);
			} else {
				return T{};
			}
		}
		static T getHalf(const T& v) {
			if constexpr(std::is_same_v<T, CalyxEngine::Vector3>) {
				return v * 0.5f;
			} else if constexpr(std::is_arithmetic_v<T>) {
				return v * T(0.5f);
			} else {
				return T{};
			}
		}

	private:
		FxValueMode mode_ = FxValueMode::Constant;

		T constant_{}; //< 定数値
		T min_{};	   //< 最小値（ランダムモード用）
		T max_{};	   //< 最大値（ランダムモード用）
	};

	template <typename T>
	FxParam<T>::FxParam() = default;

	/////////////////////////////////////////////////////////////////////////////////////////
	//		定数の設定
	/////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline void FxParam<T>::SetConstant(const T& value) {
		mode_	  = FxValueMode::Constant;
		constant_ = value;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		ランダム数の設定
	/////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline void FxParam<T>::SetRandom(const T& min, const T& max) {
		mode_ = FxValueMode::Random;
		min_  = min;
		max_  = max;
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		値の取得（スカラー型用）
	/////////////////////////////////////////////////////////////////////////////////////////
	template <typename T>
	inline T FxParam<T>::Get() const {
		if(mode_ == FxValueMode::Constant) {
			return constant_;
		} else {
			return Random::Generate<T>(min_, max_);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	//		値の取得（CalyxEngine::Vector3 専用特殊化）
	/////////////////////////////////////////////////////////////////////////////////////////
	template <>
	inline CalyxEngine::Vector3 FxParam<CalyxEngine::Vector3>::Get() const {
		switch(mode_) {
		case FxValueMode::Constant:
			return constant_;
		case FxValueMode::Random: // 立方体ランダム
			return Random::GenerateVector3(min_, max_);
		case FxValueMode::RandomSphere: { // 球状ランダム
			CalyxEngine::Vector3 center = (min_ + max_) * 0.5f;
			float			   radius = ((max_ - min_).Length()) * 0.5f;
			CalyxEngine::Vector3 dir	  = Random::GenerateUnitVector3();
			float			   dist	  = Random::Generate<float>(0.0f, radius);
			return center + dir * dist;
		}
		default:
			return constant_;
		}
	}

	//===================================================================*/
	//		Config からの設定
	//===================================================================*/
	template <typename T>
	inline void FxParam<T>::FromConfig(const FxParamConfig<T>& cfg) {
		mode_	  = cfg.mode;
		constant_ = cfg.constant;
		min_	  = cfg.min;
		max_	  = cfg.max;
	}

	//===================================================================*/
	//		Config への変換
	//===================================================================*/
	template <typename T>
	inline FxParamConfig<T> FxParam<T>::ToConfig() const { return FxParamConfig<T>{mode_, constant_, min_, max_}; }

	namespace ImGuiHelpers {

		inline bool DrawModeCombo(const char* id, FxValueMode& m) {
			int			cur		= (m == FxValueMode::Constant) ? 0 : (m == FxValueMode::Random ? 1 : 2);
			const char* items[] = {"Constant", "Random(Box)", "RandomSphere"};
			bool		changed = false;
			if(ImGui::Combo(id, &cur, items, IM_ARRAYSIZE(items))) {
				m		= (cur == 0) ? FxValueMode::Constant : (cur == 1 ? FxValueMode::Random : FxValueMode::RandomSphere);
				changed = true;
			}
			return changed;
		}

		inline bool DrawFxParamGui(const char* label, FxParam<float>& p) {
			bool changed = false;
			ImGui::PushID(label);
			FxValueMode mode = p.GetMode();

			FxGui::RowLabel(label);
			// 1行目：モード
			{
				ImGui::BeginGroup();
				changed |= DrawModeCombo("##mode", mode);
				// モード確定後に値ブロック
				if(mode == FxValueMode::Constant) {
					float v = p.GetConstant();
					if(GuiCmd::DragFloat("Value", v)) {
						p.SetConstant(v);
						changed = true;
					}
				} else { // Random
					float mn = p.GetMin(), mx = p.GetMax();
					bool  e1 = GuiCmd::DragFloat("Min", mn);
					bool  e2 = GuiCmd::DragFloat("Max", mx);
					if(e1 || e2) {
						if(mn > mx) std::swap(mn, mx);
						p.SetRandom(mn, mx);
						changed = true;
					}
				}
				ImGui::EndGroup();
			}

			// モードを反映
			if(mode != p.GetMode()) {
				if(mode == FxValueMode::Constant)
					p.SetConstant(p.GetConstant());
				else
					p.SetRandom(p.GetMin(), p.GetMax());
				changed = true;
			}
			ImGui::PopID();
			return changed;
		}

		inline bool DrawFxParamGui(const char* label, FxParam<CalyxEngine::Vector3>& p) {
			bool changed = false;
			ImGui::PushID(label);
			FxValueMode mode = p.GetMode();

			FxGui::RowLabel(label);
			ImGui::BeginGroup();
			changed |= DrawModeCombo("##mode", mode);

			if(mode == FxValueMode::Constant) {
				CalyxEngine::Vector3 v = p.GetConstant();
				if(GuiCmd::DragFloat3("Value", v)) {
					p.SetConstant(v);
					changed = true;
				}
			} else if(mode == FxValueMode::Random) {
				CalyxEngine::Vector3 mn = p.GetMin(), mx = p.GetMax();
				bool			   e1 = GuiCmd::DragFloat3("Min", mn);
				bool			   e2 = GuiCmd::DragFloat3("Max", mx);
				if(e1 || e2) {
					if(mn.x > mx.x) std::swap(mn.x, mx.x);
					if(mn.y > mx.y) std::swap(mn.y, mx.y);
					if(mn.z > mx.z) std::swap(mn.z, mx.z);
					p.SetRandom(mn, mx);
					changed = true;
				}
			} else { // RandomSphere
				// ユーザーにとって直感的な Center+Radius 入力にする
				CalyxEngine::Vector3 center = (p.GetMin() + p.GetMax()) * 0.5f;
				CalyxEngine::Vector3 half	  = (p.GetMax() - p.GetMin()) * 0.5f;
				float			   radius = half.Length();
				bool			   e1	  = GuiCmd::DragFloat3("Center", center);
				bool			   e2	  = GuiCmd::DragFloat("Radius", radius, 0.01f, 0.0f, 1e6f);
				if(e1 || e2) {
					CalyxEngine::Vector3 mn = center - CalyxEngine::Vector3(radius, radius, radius);
					CalyxEngine::Vector3 mx = center + CalyxEngine::Vector3(radius, radius, radius);
					p.SetRandom(mn, mx);
					changed = true;
				}
			}
			ImGui::EndGroup();

			// 反映
			if(mode != p.GetMode()) {
				if(mode == FxValueMode::Constant)
					p.SetConstant(p.GetConstant());
				else
					p.SetRandom(p.GetMin(), p.GetMax()); // RandomSphere も min/max を使う実装のためOK
				changed = true;
			}
			ImGui::PopID();
			return changed;
		}

	} // namespace ImGuiHelpers
}


