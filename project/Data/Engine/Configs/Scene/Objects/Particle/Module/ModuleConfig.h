#pragma once

#include <string>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <externals/nlohmann/json.hpp>

struct BaseModuleConfig {
	std::string name;
	bool enabled = true;

	BaseModuleConfig() = default;

	BaseModuleConfig(const std::string& name_, bool enabled_)
		: name(name_), enabled(enabled_) {}

	virtual ~BaseModuleConfig() = default;

	virtual nlohmann::json ToJson() const = 0;
	virtual void FromJson(const nlohmann::json& j) = 0;
};


//============================================================
// シンプルな名前と有効フラグのみ保持する Config
//============================================================
struct SimpleModuleConfig : public BaseModuleConfig {
	SimpleModuleConfig(const std::string& name_, bool enabled_)
		: BaseModuleConfig(name_, enabled_) {}

	nlohmann::json ToJson() const override {
		return {
			{ "name", name },
			{ "enabled", enabled }
		};
	}

	void FromJson(const nlohmann::json& j) override {
		if (j.contains("enabled")) j.at("enabled").get_to(enabled);
		if (j.contains("name")) j.at("name").get_to(name);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// 重力適用モジュール
/////////////////////////////////////////////////////////////////////////////////////////
struct GravityModuleConfig : public BaseModuleConfig {
	Vector3 gravity{ 0.0f, -9.8f, 0.0f };

	GravityModuleConfig() {
		name = "GravityModule";
	}

	GravityModuleConfig(const std::string& _name, bool _enable)
		: BaseModuleConfig(_name, _enable) {}

	nlohmann::json ToJson() const override {
		return {
			{ "name", name },
			{ "enabled", enabled },
			{ "gravity", gravity }
		};
	}

	void FromJson(const nlohmann::json& j) override {
		if (j.contains("enabled")) j.at("enabled").get_to(enabled);
		if (j.contains("gravity")) j.at("gravity").get_to(gravity);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// lifeTimeでサイズ変更
/////////////////////////////////////////////////////////////////////////////////////////
struct SizeOverLifetimeConfig 
	: public BaseModuleConfig {
	bool isGrowing = true;
	Cx::Ease::EaseType easeType =	 Cx::Ease::EaseType::EaseInOutCubic;

	SizeOverLifetimeConfig() {
		name = "SizeOverLifetimeModule";
	}

	SizeOverLifetimeConfig(const std::string& _name, bool _enable)
		: BaseModuleConfig(_name, _enable) {}

	nlohmann::json ToJson() const override {
		return {
			{ "name", name },
			{ "enabled", enabled },
			{ "isGrowing", isGrowing },
			{ "easeType", static_cast<int>(easeType) }
		};
	}

	void FromJson(const nlohmann::json& j) override {
		if (j.contains("enabled")) j.at("enabled").get_to(enabled);
		if (j.contains("isGrowing")) j.at("isGrowing").get_to(isGrowing);
		if (j.contains("easeType")) {
			int ease = 0;
			j.at("easeType").get_to(ease);
			easeType = static_cast<Cx::Ease::EaseType>(ease);
		}
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// lifeTimeに応じてパラメータの調整
/////////////////////////////////////////////////////////////////////////////////////////
struct OverLifetimeModuleConfig : public BaseModuleConfig {
	OverLifetimeModuleConfig() { name = "OverLifetimeModule"; }
	OverLifetimeModuleConfig(const std::string& _name, bool _enable)
		: BaseModuleConfig(_name, _enable) {}

	// Target/Blend/Ease は int で保存（モジュール側の enum と対応）
	int  target  = 0;  // 0:Scale, 1:RotX, 2:RotY, 3:RotZ, 4:ColorRGBA, 5:AlphaOnly
	int  blend   = 0;  // 0:Set, 1:Add, 2:Multiply
	int  ease    = static_cast<int>(Cx::Ease::EaseType::EaseInOutCubic);
	bool clamp01 = true;
	bool invert  = false;

	// start/end は Vector4
	Vector4 start {0,0,0,1};
	Vector4 end   {1,1,1,1};

	nlohmann::json ToJson() const override {
		return {
	            {"name", name},
				{"enabled", enabled},
				{"target", target},
				{"blend",  blend},
				{"ease",   ease},
				{"clamp01", clamp01},
				{"invert",  invert},
				{"start",   {start.x, start.y, start.z, start.w}},
				{"end",     {end.x,   end.y,   end.z,   end.w}},
			};
	}

	void FromJson(const nlohmann::json& j) override {
		if (j.contains("enabled")) j.at("enabled").get_to(enabled);
		if (j.contains("target"))  j.at("target").get_to(target);
		if (j.contains("blend"))   j.at("blend").get_to(blend);
		if (j.contains("ease"))    j.at("ease").get_to(ease);
		if (j.contains("clamp01")) j.at("clamp01").get_to(clamp01);
		if (j.contains("invert"))  j.at("invert").get_to(invert);
		if (j.contains("start")) { auto a=j.at("start"); start={a.at(0),a.at(1),a.at(2),a.at(3)}; }
		if (j.contains("end"))   { auto a=j.at("end");   end  ={a.at(0),a.at(1),a.at(2),a.at(3)}; }
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// uvAnimation
/////////////////////////////////////////////////////////////////////////////////////////
struct TextureSheetAnimationConfig 
	: public BaseModuleConfig{
	int rows = 4;
	int cols = 4;
	bool loop = true;
	float animationSpeed = 10.0f;
	bool useCustomFrames = false;

	TextureSheetAnimationConfig(){
		name = "TextureSheetAnimationModule";
	}

	TextureSheetAnimationConfig(const std::string& _name, bool _enable)
		: BaseModuleConfig(_name, _enable){}

	nlohmann::json ToJson() const override{
		return {
			{ "name", name },
			{ "enabled", enabled },
			{ "rows", rows },
			{ "cols", cols },
			{ "loop", loop },
			{ "animationSpeed", animationSpeed },
			{ "useCustomFrames", useCustomFrames }
			// カスタムUVリスト（オプション）があればここに追加可能
		};
	}

	void FromJson(const nlohmann::json& j) override{
		if (j.contains("enabled")) j.at("enabled").get_to(enabled);
		if (j.contains("rows")) j.at("rows").get_to(rows);
		if (j.contains("cols")) j.at("cols").get_to(cols);
		if (j.contains("loop")) j.at("loop").get_to(loop);
		if (j.contains("animationSpeed")) j.at("animationSpeed").get_to(animationSpeed);
		if (j.contains("useCustomFrames")) j.at("useCustomFrames").get_to(useCustomFrames);
	}
};