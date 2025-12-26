#include "OverLifetimeModule.h"

#include "Engine/Foundation/Math/MathUtil.h"

#include <externals/imgui/imgui.h>

namespace CalyxEffect {
	OverLifetimeModule::OverLifetimeModule(const std::string& name)
		: BaseFxModule(name) {}

	void OverLifetimeModule::OnUpdate(FxUnit& unit, float /*dt*/) {
		// 進行度 t：lifeT を優先、無ければ age/lifetime で算出
		float t = 1.0f;
		if(unit.lifetime > 0.0f) {
			if(unit.lifeT >= 0.0f && unit.lifeT <= 1.0f)
				t = unit.lifeT;
			else
				t = unit.age / unit.lifetime;
		}
		if(clamp01_) t = std::clamp(t, 0.0f, 1.0f);
		if(invert_) t = 1.0f - t;

		const float et = CalyxEase::ApplyEase(ease_, t);

		switch(target_) {
		case Target::Scale: {
			CalyxMath::Vector3 s{start_.x, start_.y, start_.z};
			CalyxMath::Vector3 e{end_.x, end_.y, end_.z};
			CalyxMath::Vector3 v = CalyxMath::Vector3::Lerp(s, e, et);
			ApplyTo(unit, {v.x, v.y, v.z, 1.0f});
		} break;

		case Target::RotationX:
		case Target::RotationY:
		case Target::RotationZ:
		case Target::AlphaOnly: {
			float v = CalyxMath::Lerp(start_.x, end_.x, et);
			ApplyTo(unit, {v, 0, 0, 0});
		} break;

		case Target::ColorRGBA: {
			CalyxMath::Vector4 v = CalyxMath::Vector4::Lerp(start_, end_, et);
			ApplyTo(unit, v);
		} break;
		}
	}

	void OverLifetimeModule::ApplyTo(FxUnit& u, const CalyxMath::Vector4& v) const {
		switch(target_) {
		case Target::Scale: {
			CalyxMath::Vector3 cur = u.scale;
			CalyxMath::Vector3 nv	= {v.x, v.y, v.z};
			switch(blend_) {
			case BlendOp::Set:
				u.scale = nv;
				break;
			case BlendOp::Add:
				u.scale = {cur.x + nv.x, cur.y + nv.y, cur.z + nv.z};
				break;
			case BlendOp::Multiply:
				u.scale = {cur.x * nv.x, cur.y * nv.y, cur.z * nv.z};
				break;
			}
		} break;

		case Target::RotationX: {
			float cur = u.rotationEuler.x, nv = v.x;
			switch(blend_) {
			case BlendOp::Set:
				u.rotationEuler.x = nv;
				break;
			case BlendOp::Add:
				u.rotationEuler.x = cur + nv;
				break;
			case BlendOp::Multiply:
				u.rotationEuler.x = cur * nv;
				break;
			}
		} break;

		case Target::RotationY: {
			float cur = u.rotationEuler.y, nv = v.x;
			switch(blend_) {
			case BlendOp::Set:
				u.rotationEuler.y = nv;
				break;
			case BlendOp::Add:
				u.rotationEuler.y = cur + nv;
				break;
			case BlendOp::Multiply:
				u.rotationEuler.y = cur * nv;
				break;
			}
		} break;

		case Target::RotationZ: {
			float cur = u.rotationEuler.z, nv = v.x;
			switch(blend_) {
			case BlendOp::Set:
				u.rotationEuler.z = nv;
				break;
			case BlendOp::Add:
				u.rotationEuler.z = cur + nv;
				break;
			case BlendOp::Multiply:
				u.rotationEuler.z = cur * nv;
				break;
			}
		} break;

		case Target::ColorRGBA: {
			CalyxMath::Vector4 cur = u.color, nv = v;
			switch(blend_) {
			case BlendOp::Set:
				u.color = nv;
				break;
			case BlendOp::Add:
				u.color = {cur.x + nv.x, cur.y + nv.y, cur.z + nv.z, cur.w + nv.w};
				break;
			case BlendOp::Multiply:
				u.color = {cur.x * nv.x, cur.y * nv.y, cur.z * nv.z, cur.w * nv.w};
				break;
			}
		} break;

		case Target::AlphaOnly: {
			float cur = u.color.w, nv = v.x;
			switch(blend_) {
			case BlendOp::Set:
				u.color.w = nv;
				break;
			case BlendOp::Add:
				u.color.w = cur + nv;
				break;
			case BlendOp::Multiply:
				u.color.w = cur * nv;
				break;
			}
		} break;
		}
	}

	void OverLifetimeModule::DrawValueEditor(const char* label, CalyxMath::Vector4& v) {
		ImGui::TextUnformatted(label);
		ImGui::PushID(label);
		switch(target_) {
		case Target::Scale: {
			float a[3] = {v.x, v.y, v.z};
			ImGui::DragFloat3("##v3", a, 0.01f, -FLT_MAX, FLT_MAX);
			v.x = a[0];
			v.y = a[1];
			v.z = a[2];
		} break;
		case Target::RotationX:
		case Target::RotationY:
		case Target::RotationZ:
		case Target::AlphaOnly: {
			ImGui::DragFloat("##f", &v.x, 0.1f, -FLT_MAX, FLT_MAX);
		} break;
		case Target::ColorRGBA: {
			float arr[4] = {v.x, v.y, v.z, v.w};
			ImGui::ColorEdit4("##col", arr, ImGuiColorEditFlags_Float);
			v.x = arr[0];
			v.y = arr[1];
			v.z = arr[2];
			v.w = arr[3];
		} break;
		}
		ImGui::PopID();
	}

	void OverLifetimeModule::ShowGuiContent() {
		ImGui::SeparatorText("Over Lifetime");

		// Target
		{
			ImGui::TextUnformatted("Target");
			static const char* items =
				"Scale\0RotationX\0RotationY\0RotationZ\0ColorRGBA\0AlphaOnly\0";
			int t = static_cast<int>(target_);
			if(ImGui::Combo("##target", &t, items)) {
				target_ = static_cast<Target>(t);
			}
		}

		// Blend
		{
			ImGui::Spacing();
			ImGui::TextUnformatted("Blend");
			int b = static_cast<int>(blend_);
			if(ImGui::Combo("##blend", &b, "Set\0Add\0Multiply\0")) {
				blend_ = static_cast<BlendOp>(b);
			}
		}

		// Ease
		{
			ImGui::Spacing();
			ImGui::TextUnformatted("Ease");
			// 実装側の列挙に合わせて表示内容は調整してください
			int et = static_cast<int>(ease_);
			if(ImGui::Combo("##ease", &et,
							"Linear\0EaseIn\0EaseOut\0EaseInOut\0EaseInOutCubic\0")) {
				ease_ = static_cast<CalyxEase::EaseType>(et);
							}
		}

		// Values
		ImGui::Spacing();
		DrawValueEditor("Start", start_);
		DrawValueEditor("End", end_);

		// Flags
		ImGui::Spacing();
		ImGui::Checkbox("Clamp 0..1", &clamp01_);
		ImGui::SameLine();
		ImGui::Checkbox("Invert (1 - t)", &invert_);

		ImGui::Spacing();
		ImGui::TextDisabled("Rotation values are treated as degrees.");
	}
}