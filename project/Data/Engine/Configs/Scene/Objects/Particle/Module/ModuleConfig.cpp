#include "ModuleConfig.h"

#include <Engine/Application/Effects/Particle/Module/OverLifetime/OverLifetimeModule.h>

namespace CalyxEffect {
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
		if(j.contains("guid")) { j.at("guid").get_to(guid); }
		if(j.contains("enabled")) j.at("enabled").get_to(enabled);
		if(j.contains("target")) j.at("target").get_to(target);
		if(j.contains("blend")) j.at("blend").get_to(blend);
		if(j.contains("ease")) j.at("ease").get_to(ease);
		if(j.contains("clamp01")) j.at("clamp01").get_to(clamp01);
		if(j.contains("invert")) j.at("invert").get_to(invert);

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
		m.SetEnabled(enabled);
		m.SetTarget(static_cast<OverLifetimeModule::Target>(target));
		m.SetBlend(static_cast<OverLifetimeModule::BlendOp>(blend));
		m.SetEaseType(static_cast<CalyxEase::EaseType>(ease));
		m.SetClamp01(clamp01);
		m.SetInvert(invert);
		m.SetStart(start);
		m.SetEnd(end);
	}

	void OverLifetimeModuleConfig::ExtractFrom(const OverLifetimeModule& m) {
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