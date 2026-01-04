#pragma once

#include <string>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Ease/EaseTypes.h>
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
	EaseType easeType = EaseType::EaseInOutCubic;

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
			easeType = static_cast<EaseType>(ease);
		}
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

