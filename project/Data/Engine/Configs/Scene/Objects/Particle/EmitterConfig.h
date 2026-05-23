#pragma once

#include "Engine/Graphics/Pipeline/BlendMode/BlendMode.h"
#include <Engine/Application/Effects/Particle/Emitter/EmitterDetails.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/FxParmConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfigFactory.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <engine/Foundation/Utility/Guid/Guid.h>
#include <externals/nlohmann/json.hpp>
#include <string>
#include <vector>

namespace CalyxEngine {
	struct EmitterConfig {
		CalyxEngine::Vector3 position{};
		CalyxEngine::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		Vector3ParamConfig scale;
		Vector3ParamConfig velocity;
		FxFloatParamConfig lifetime;

		float		emitRate	= 0.1f;
		std::string modelPath	= "plane.obj";
		std::string texturePath = "Textures/white1x1.dds";

		Guid textureGuid{Guid::Empty()};
		Guid modelGuid{Guid::Empty()};

		bool		  isDrawEnable	 = true;
		bool		  followOneShot	 = false;
		bool		  isComplement	 = true;
		bool		  randomSpinEmit = false;
		bool          cameraDitherEnabled = false;
		BillboardMode billboardMode	 = BillboardMode::Full;
		BlendMode	  blendMode		 = BlendMode::ADD;
		EmitterShape emitterShape	 = EmitterShape::Point;
		float         cameraDitherNear = 0.0f;
		float         cameraDitherFar  = 20.0f;
		// 既存データ互換: キー未定義時は BaseEmitter と同じ既定値を使う
		CalyxEngine::Vector3 shapeSize{1.0f, 1.0f, 1.0f};
		float shapeRadius = 1.0f;
		float shapeAngle  = 30.0f;

		// 再生・OneShot制御関連
		bool  isOneShot	   = false;
		bool  autoDestroy  = false;
		int	  emitCount	   = 10;
		float emitDelay	   = 0.0f;
		float emitDuration = -1.0f;

		std::vector<std::unique_ptr<BaseModuleConfig>> modules;

		void		   FromJson(const nlohmann::json& j);
		nlohmann::json ToJson() const;
	};

	inline void EmitterConfig::FromJson(const nlohmann::json& j) {
		position	   = j.value("position", CalyxEngine::Vector3{0, 0, 0});
		scale		   = j.value("scale", Vector3ParamConfig{});
		color		   = j.value("color", CalyxEngine::Vector4{1, 1, 1, 1});
		velocity	   = j.value("velocity", Vector3ParamConfig{});
		lifetime	   = j.value("lifetime", FxFloatParamConfig{});
		emitRate	   = j.value("emitRate", 1.0f);
		modelPath	   = j.value("modelPath", "plane.obj");
		texturePath	   = j.value("texturePath", "Textures/white1x1.dds");
		isDrawEnable   = j.value("isDrawEnable", true);
		isComplement   = j.value("isComplement", true);
		randomSpinEmit = j.value("randomSpinEmit", false);
		followOneShot  = j.value("followOneShot", false);
		cameraDitherEnabled = j.value("cameraDitherEnabled", false);
		cameraDitherNear = j.value("cameraDitherNear", 0.0f);
		cameraDitherFar = j.value("cameraDitherFar", 20.0f);
		billboardMode = j.value("billboardMode", BillboardMode::Full);
		emitterShape   = j.value("emitterShape", EmitterShape::Point);
		shapeSize      = j.value("shapeSize", CalyxEngine::Vector3{1.0f, 1.0f, 1.0f});
		shapeRadius    = j.value("shapeRadius", 1.0f);
		shapeAngle     = j.value("shapeAngle", 30.0f);

		// 互換のため両方のキーを受け入れる
		if(auto it = j.find("textureGuid"); it != j.end() && !it->is_null()) {
			textureGuid = it->get<Guid>();
			// すべて大文字のキー
		} else if(auto it2 = j.find("textureGUID"); it2 != j.end() && !it2->is_null()) {
			textureGuid = it2->get<Guid>();
			// guidがない場合0で初期化
		} else {
			textureGuid = Guid::Empty();
		}

		if(auto it = j.find("modelGuid"); it != j.end() && !it->is_null()) {
			modelGuid = it->get<Guid>();
		} else if(auto it2 = j.find("modelGUID"); it2 != j.end() && !it2->is_null()) {
			modelGuid = it2->get<Guid>();
		} else {
			modelGuid = Guid::Empty();
		}

		// 再生・OneShot制御
		isOneShot	 = j.value("isOneShot", false);
		autoDestroy	 = j.value("autoDestroy", false);
		emitCount	 = j.value("emitCount", 10);
		emitDelay	 = j.value("emitDelay", 0.0f);
		emitDuration = j.value("emitDuration", -1.0f);
		blendMode	 = j.value("blendMode", BlendMode::ADD);

		// モジュール
		modules.clear();
		if(j.contains("modules") && j["modules"].is_array()) {
			for(const auto& moduleJson : j["modules"]) {
				auto modConfig = ModuleConfigFactory::FromJson(moduleJson);
				if(modConfig) modules.push_back(std::move(modConfig));
			}
		}
	}

	inline nlohmann::json EmitterConfig::ToJson() const {
		nlohmann::json j;
		j["position"]		= position;
		j["color"]			= color;
		j["velocity"]		= velocity;
		j["scale"]			= scale;
		j["lifetime"]		= lifetime;
		j["emitRate"]		= emitRate;
		j["modelPath"]		= modelPath;
		j["texturePath"]	= texturePath;
		j["isDrawEnable"]	= isDrawEnable;
		j["isComplement"]	= isComplement;
		j["randomSpinEmit"] = randomSpinEmit;
		j["followOneShot"]	= followOneShot;
		j["cameraDitherEnabled"] = cameraDitherEnabled;
		j["cameraDitherNear"] = cameraDitherNear;
		j["cameraDitherFar"] = cameraDitherFar;
		j["billboardMode"] = billboardMode;
		j["blendMode"]		= blendMode;
		j["emitterShape"]  = emitterShape;
		j["shapeSize"]     = shapeSize;
		j["shapeRadius"]   = shapeRadius;
		j["shapeAngle"]    = shapeAngle;

		// 再生・OneShot制御
		j["isOneShot"]	  = isOneShot;
		j["autoDestroy"]  = autoDestroy;
		j["emitCount"]	  = emitCount;
		j["emitDelay"]	  = emitDelay;
		j["emitDuration"] = emitDuration;

		// モジュール
		j["modules"] = nlohmann::json::array();
		for(const auto& mod : modules) j["modules"].push_back(mod->ToJson());

		j["textureGuid"] = textureGuid;
		j["modelGuid"]	 = modelGuid;

		return j;
	}

	inline void to_json(nlohmann::json& j, const EmitterConfig& c) { j = c.ToJson(); }
	inline void from_json(const nlohmann::json& j, EmitterConfig& c) { c.FromJson(j); }
} // namespace CalyxEngine
