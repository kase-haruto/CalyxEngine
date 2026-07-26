#include "ModuleConfig.h"

#include <Engine/Application/Effects/Particle/Module/OverLifetime/OverLifetimeModule.h>

namespace CalyxEngine {
	nlohmann::json LifetimeModuleConfig::ToJson() const {
		return {{"guid", guid}, {"name", name}, {"enabled", enabled}, {"stage", stage},
			{"target", target}, {"floatCurve", floatCurve}, {"vectorCurve", vectorCurve}, {"gradient", gradient}};
	}

	void LifetimeModuleConfig::FromJson(const nlohmann::json& j) {
		// 欠落Keyには既定値または現在値を使い、旧Particle Assetとの互換性を維持する。
		guid = j.value("guid", Guid::Empty());
		name = j.value("name", name);
		enabled = j.value("enabled", true);
		stage = j.value("stage", ParticleModuleStage::Update);
		target = j.value("target", target);
		floatCurve = j.value("floatCurve", FloatCurve{});
		vectorCurve = j.value("vectorCurve", Vector3Curve{});
		gradient = j.value("gradient", ColorGradient{});
	}

	nlohmann::json OverLifetimeModuleConfig::ToJson() const {
		return {
				{"guid",guid},
				{"name",name},
				{"enabled",enabled},
				{"target",target},
				{"blend",blend},
				{"ease",ease},
				{"clamp01",clamp01},
				{"invert",invert},
				{"start",{start.x,start.y,start.z,start.w}},
				{"end",{end.x,end.y,end.z,end.w}},
			};
	}

	void OverLifetimeModuleConfig::FromJson(const nlohmann::json& j) {
		// 旧形式で存在しない設定は初期値を維持するため、各Keyを個別に確認する。
		if(j.contains("guid")) { j.at("guid").get_to(guid); }
		if(j.contains("enabled")) j.at("enabled").get_to(enabled);
		if(j.contains("target")) j.at("target").get_to(target);
		if(j.contains("blend")) j.at("blend").get_to(blend);
		if(j.contains("ease")) j.at("ease").get_to(ease);
		if(j.contains("clamp01")) j.at("clamp01").get_to(clamp01);
		if(j.contains("invert")) j.at("invert").get_to(invert);

		// Vector4はJSON配列として保存されるため、4成分を順序通り復元する。
		if(j.contains("start")) {
			auto a = j.at("start");
			start  = {a.at(0),a.at(1),a.at(2),a.at(3)};
		}
		if(j.contains("end")) {
			auto a = j.at("end");
			end    = {a.at(0),a.at(1),a.at(2),a.at(3)};
		}
	}

	void OverLifetimeModuleConfig::ApplyTo(OverLifetimeModule& m) const {
		// 永続化用の整数enumをRuntime Moduleの型へ変換し、全設定を一括適用する。
		m.SetEnabled(enabled);
		m.SetTarget(static_cast<OverLifetimeModule::Target>(target));
		m.SetBlend(static_cast<OverLifetimeModule::BlendOp>(blend));
		m.SetEaseType(static_cast<CalyxEngine::EaseType>(ease));
		m.SetClamp01(clamp01);
		m.SetInvert(invert);
		m.SetStart(start);
		m.SetEnd(end);
	}

	void OverLifetimeModuleConfig::ExtractFrom(const OverLifetimeModule& m) {
		// Editorで変更されたRuntime値を保存Configへ戻し、次回Loadでも同じ挙動を再現する。
		name    = m.GetName();
		enabled = m.IsEnabled();
		target  = static_cast<int>(m.GetTarget());
		blend   = static_cast<int>(m.GetBlend());
		ease    = static_cast<int>(m.GetEaseType());
		clamp01 = m.GetClamp01();
		invert  = m.GetInvert();
		start   = m.GetStart();
		end     = m.GetEnd();
	}
}
