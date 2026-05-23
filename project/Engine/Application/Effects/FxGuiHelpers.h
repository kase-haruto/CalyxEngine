#pragma once
#include <Data/Engine/Configs/Scene/Objects/Particle/FxParmConfig.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>

struct CalyxEngine::Vector3;

namespace CalyxEngine {

	enum class FxValueMode;
	template <class T>
	class FxParam;

	namespace FxGui {

		//---------------------- 2列グリッド ----------------------
		struct GridScope {
			bool open		 = false; // セクションが開いていて、かつテーブル生成に成功したか
			bool began_table = false; // BeginTable できたか（EndTable の判定用）
			GridScope(const char* label, bool defaultOpen = false);
			~GridScope();
		};

		struct FullWidthScope {
			FullWidthScope() { ImGui::PushItemWidth(-FLT_MIN); }
			~FullWidthScope() { ImGui::PopItemWidth(); }
		};

		// 行ラベル（テーブル外でも安全に動く）
		void RowLabel(const char* name);

		//---------------------- モード選択 ----------------------
		bool DrawModeCombo(const char* id, FxValueMode& m);

		//---------------------- Param 共通（テンプレ＋特殊化） ----------------------
		template <class T>
		bool DrawParamEditor(const char* /*id*/, T& /*dummy*/);

		template <>
		inline bool DrawParamEditor<float>(const char* id, float& v) {
			return GuiCmd::DragFloat(id, v);
		}
		template <>
		inline bool DrawParamEditor<CalyxEngine::Vector3>(const char* id, CalyxEngine::Vector3& v) {
			return GuiCmd::DragFloat3(id, v);
		}

		template <class T>
		bool DrawParam(const char* label, FxParam<T>& p) {
			bool changed = false;
			ImGui::PushID(label);

			FxValueMode mode = p.GetMode();
			FxGui::RowLabel(label);
			ImGui::BeginGroup();

			bool modeChanged = FxGui::DrawModeCombo("##mode", mode);

			if(mode == FxValueMode::Constant) {
				T v = p.GetConstant();
				if(DrawParamEditor<T>("Value", v)) {
					p.SetConstant(v);
					changed = true;
				}
			} else if constexpr(std::is_same_v<T, float>) {
				float mn = p.GetMin(), mx = p.GetMax();
				bool  e1 = GuiCmd::DragFloat("Min", mn);
				bool  e2 = GuiCmd::DragFloat("Max", mx);
				if(e1 || e2) {
					if(mn > mx) std::swap(mn, mx);
					p.SetRandom(mn, mx);
					changed = true;
				}
			} else if constexpr(std::is_same_v<T, CalyxEngine::Vector3>) {
				if(mode == FxValueMode::Random) {
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
				} else { // RandomSphere (Center + Radius)
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
			}

			if(modeChanged) {
				if(mode == FxValueMode::Constant)
					p.SetConstant(p.GetConstant());
				else
					p.SetRandom(p.GetMin(), p.GetMax());
				changed = true;
			}

			ImGui::EndGroup();
			ImGui::PopID();
			return changed;
		}

	} // namespace FxGui
} // namespace CalyxEngine
