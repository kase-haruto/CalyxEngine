#pragma once

#include "Engine/Graphics/Pipeline/BlendMode/BlendMode.h"
#include <Engine/Application/Effects/Particle/Emitter/EmitterDetails.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/FxParmConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfig.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/Module/ModuleConfigFactory.h>
#include <Data/Engine/Configs/Scene/Objects/Particle/TrailConfig.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Quaternion.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <engine/Foundation/Utility/Guid/Guid.h>
#include <externals/nlohmann/json.hpp>
#include <algorithm>
#include <string>
#include <vector>

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * ParticleUVSettings
	 * - Particle描画で使用するUV変換設定を保持するデータ構造
	 * - Offset、Tiling、Scroll速度、回転角を保存する
	 * - Editor UIやGPUリソースそのものは管理しない
	 *---------------------------------------------------------------------------------------*/
	struct ParticleUVSettings {
		Vector2 offset{0.0f,0.0f};       //< UVの固定オフセット
		Vector2 tiling{1.0f,1.0f};       //< UVの拡大率
		Vector2 scrollSpeed{0.0f,0.0f};  //< 1秒あたりのUV移動量
		float rotation = 0.0f;            //< UV中心を基準とした回転角度（radian）
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ParticleUVSettings, offset, tiling, scrollSpeed, rotation)

	/*-----------------------------------------------------------------------------------------
	 * DirectionConfig
	 * - Particle生成時の射出方向と速度を保持する設定データ構造
	 * - 方向への姿勢追従を行うかどうかも管理する
	 *---------------------------------------------------------------------------------------*/
	struct DirectionConfig {
		bool enabled = false;
		Vector3ParamConfig vector;
		FxFloatParamConfig speed;
		bool rotateToDirection = false;

		DirectionConfig() {
			vector.constant = CalyxEngine::Vector3(0.0f,1.0f,0.0f);
			vector.min = CalyxEngine::Vector3(-1.0f,0.0f,-1.0f);
			vector.max = CalyxEngine::Vector3(1.0f,0.0f,1.0f);
			speed.constant = 1.0f;
			speed.min = 0.0f;
			speed.max = 1.0f;
		}
	};

	inline void to_json(nlohmann::json& j,const DirectionConfig& c) {
		j = nlohmann::json{
			{"enabled",c.enabled},
			{"vector",c.vector},
			{"speed",c.speed},
			{"rotateToDirection",c.rotateToDirection}};
	}

	inline void from_json(const nlohmann::json& j,DirectionConfig& c) {
		c.enabled = j.value("enabled",false);
		if(auto it = j.find("vector"); it != j.end() && !it->is_null()) {
			c.vector = it->get<Vector3ParamConfig>();
		}
		if(auto it = j.find("speed"); it != j.end() && !it->is_null()) {
			c.speed = it->get<FxFloatParamConfig>();
		}
		c.rotateToDirection = j.value("rotateToDirection",false);
	}

	inline Vector3ParamConfig ReadSpinConfig(const nlohmann::json& j,const Vector3ParamConfig& fallback) {
		Vector3ParamConfig result = fallback;

		auto readSpinValue = [](const nlohmann::json& value,CalyxEngine::Vector3& out) {
			if(value.is_number()) {
				out = CalyxEngine::Vector3(0.0f,0.0f,value.get<float>());
			} else {
				out = value.get<CalyxEngine::Vector3>();
			}
		};

		if(j.is_null()) return result;
		if(j.is_number()) {
			result.mode = FxValueMode::Constant;
			result.constant = CalyxEngine::Vector3(0.0f,0.0f,j.get<float>());
			return result;
		}

		if(!j.is_object()) return j.get<Vector3ParamConfig>();

		if(auto it = j.find("mode"); it != j.end() && !it->is_null()) {
			result.mode = it->get<FxValueMode>();
		}
		if(auto it = j.find("constant"); it != j.end() && !it->is_null()) {
			readSpinValue(*it,result.constant);
		}
		if(auto it = j.find("min"); it != j.end() && !it->is_null()) {
			readSpinValue(*it,result.min);
		}
		if(auto it = j.find("max"); it != j.end() && !it->is_null()) {
			readSpinValue(*it,result.max);
		}

		return result;
	}

	/*-----------------------------------------------------------------------------------------
	 * EmitterConfig
	 * - Particle Emitterのシリアライズ可能な全設定を保持するデータ構造
	 * - Transform、生成属性、描画Asset、Noise、Trail、Module設定を管理する
	 * - Runtime Particle、Descriptor、GPU Bufferの所有は担当しない
	 *---------------------------------------------------------------------------------------*/
	struct EmitterConfig {
		Vector3 offset;
		CalyxEngine::Vector3 position{};
		CalyxEngine::Quaternion rotation = CalyxEngine::Quaternion::MakeIdentity();
		CalyxEngine::Vector3 worldScale{1.0f, 1.0f, 1.0f};
		CalyxEngine::Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
		CalyxEngine::Vector4 vertexColor{1.0f,1.0f,1.0f,1.0f};
		ParticleUVSettings uvSettings{};
		TrailSettingsConfig trail{};
		Vector3ParamConfig scale;
		Vector3ParamConfig velocity;
		DirectionConfig direction;
		CalyxEngine::Vector3 initialRotation{0.0f,0.0f,0.0f}; //< 生成時に適用する固定回転（radian）
		Vector3ParamConfig spin;
		FxFloatParamConfig lifetime;

		float		emitRate	= 0.1f;
		std::string modelPath	= "plane.obj";
		std::string texturePath = "Textures/white1x1.dds";
		std::string noiseMaskTexturePath;
		Guid noiseMaskTextureGuid{Guid::Empty()};
		float noiseMaskScale = 1.0f;
		float noiseMaskStrength = 1.0f;
		float noiseMaskThreshold = 0.5f;
		float noiseMaskSoftness = 0.1f;
		CalyxEngine::Vector2 noiseMaskScrollSpeed{0.0f,0.0f};
		bool gpuCurlNoiseEnabled = false;
		float gpuCurlNoiseFrequency = 1.0f;
		uint32_t gpuCurlNoiseOctaves = 1;
		float gpuCurlNoiseRoughness = 0.5f;
		float gpuCurlNoiseLacunarity = 2.0f;
		float gpuCurlNoiseAmplitude = 1.0f;
		CalyxEngine::Vector3 gpuCurlNoiseOffset{};
		CalyxEngine::Vector3 gpuCurlNoiseScrollSpeed{};

		Guid textureGuid{Guid::Empty()};
		Guid modelGuid{Guid::Empty()};

		bool		  isDrawEnable	 = true;
		bool		  followOneShot	 = false;
		bool		  isComplement	 = true;
		bool		  randomSpinEmit = false;
		bool          fixedRandomSeed = false;
		uint32_t      randomSeed = 1;
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

		EmitterConfig() {
			spin.constant = CalyxEngine::Vector3(0.0f,0.0f,0.0f);
			spin.min = CalyxEngine::Vector3(0.0f,0.0f,0.0f);
			spin.max = CalyxEngine::Vector3(0.0f,0.0f,0.0f);
		}
		EmitterConfig(const EmitterConfig& other)
			: EmitterConfig() { FromJson(other.ToJson()); }
		EmitterConfig& operator=(const EmitterConfig& other) {
			if(this != &other) FromJson(other.ToJson());
			return *this;
		}
		EmitterConfig(EmitterConfig&&) noexcept = default;
		EmitterConfig& operator=(EmitterConfig&&) noexcept = default;

		void		   FromJson(const nlohmann::json& j);
		nlohmann::json ToJson() const;
	};

	inline void EmitterConfig::FromJson(const nlohmann::json& j) {
		offset = j.value("offset", Vector3(0.0f,0.0f,0.0f));
		position	   = j.value("position", CalyxEngine::Vector3{0, 0, 0});
		rotation	   = j.value("rotation", j.value("worldRotation", CalyxEngine::Quaternion::MakeIdentity()));
		worldScale	   = j.value("worldScale", CalyxEngine::Vector3{1.0f, 1.0f, 1.0f});
		scale		   = j.value("scale", Vector3ParamConfig{});
		color		   = j.value("color", CalyxEngine::Vector4{1, 1, 1, 1});
		vertexColor  = j.value("vertexColor", CalyxEngine::Vector4{1,1,1,1});
		uvSettings   = j.value("uvSettings", ParticleUVSettings{});
		trail        = j.value("trail", TrailSettingsConfig{});
		velocity	   = j.value("velocity", Vector3ParamConfig{});
		direction	   = j.value("direction", DirectionConfig{});
		initialRotation = j.value("initialRotation", CalyxEngine::Vector3{0.0f,0.0f,0.0f});
		if(auto it = j.find("spin"); it != j.end() && !it->is_null()) {
			spin = ReadSpinConfig(*it,spin);
		}
		lifetime	   = j.value("lifetime", FxFloatParamConfig{});
		emitRate	   = j.value("emitRate", 1.0f);
		modelPath	   = j.value("modelPath", "plane.obj");
		texturePath	   = j.value("texturePath", "Textures/white1x1.dds");
		noiseMaskTexturePath = j.value("noiseMaskTexturePath", std::string{});
		noiseMaskTextureGuid = j.value("noiseMaskTextureGuid", Guid::Empty());
		noiseMaskScale = (std::max)(j.value("noiseMaskScale", 1.0f), 0.01f);
		noiseMaskStrength = std::clamp(j.value("noiseMaskStrength", 1.0f), 0.0f, 1.0f);
		noiseMaskThreshold = std::clamp(j.value("noiseMaskThreshold", 0.5f), 0.0f, 1.0f);
		noiseMaskSoftness = std::clamp(j.value("noiseMaskSoftness", 0.1f), 0.0001f, 1.0f);
		noiseMaskScrollSpeed = j.value("noiseMaskScrollSpeed", CalyxEngine::Vector2{0.0f,0.0f});
		gpuCurlNoiseEnabled = j.value("gpuCurlNoiseEnabled",false);
		gpuCurlNoiseFrequency = (std::max)(j.value("gpuCurlNoiseFrequency",1.0f),0.0001f);
		gpuCurlNoiseOctaves = std::clamp(j.value("gpuCurlNoiseOctaves",1u),1u,4u);
		gpuCurlNoiseRoughness = std::clamp(j.value("gpuCurlNoiseRoughness",0.5f),0.0f,1.0f);
		gpuCurlNoiseLacunarity = (std::max)(j.value("gpuCurlNoiseLacunarity",2.0f),1.0f);
		gpuCurlNoiseAmplitude = (std::max)(j.value("gpuCurlNoiseAmplitude",1.0f),0.0f);
		gpuCurlNoiseOffset = j.value("gpuCurlNoiseOffset",CalyxEngine::Vector3{});
		gpuCurlNoiseScrollSpeed = j.value("gpuCurlNoiseScrollSpeed",CalyxEngine::Vector3{});
		isDrawEnable   = j.value("isDrawEnable", true);
		isComplement   = j.value("isComplement", true);
		randomSpinEmit = j.value("randomSpinEmit", false);
		fixedRandomSeed = j.value("fixedRandomSeed", false);
		randomSeed = j.value("randomSeed", 1u);
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
		j["offset"]			= offset;
		j["position"]		= position;
		j["rotation"]		= rotation;
		j["worldScale"]		= worldScale;
		j["color"]			= color;
		j["vertexColor"] = vertexColor;
		j["uvSettings"] = uvSettings;
		j["trail"] = trail;
		j["velocity"]		= velocity;
		j["direction"]		= direction;
		j["initialRotation"] = initialRotation;
		j["spin"]			= spin;
		j["scale"]			= scale;
		j["lifetime"]		= lifetime;
		j["emitRate"]		= emitRate;
		j["modelPath"]		= modelPath;
		j["texturePath"]	= texturePath;
		j["noiseMaskTexturePath"] = noiseMaskTexturePath;
		j["noiseMaskTextureGuid"] = noiseMaskTextureGuid;
		j["noiseMaskScale"] = noiseMaskScale;
		j["noiseMaskStrength"] = noiseMaskStrength;
		j["noiseMaskThreshold"] = noiseMaskThreshold;
		j["noiseMaskSoftness"] = noiseMaskSoftness;
		j["noiseMaskScrollSpeed"] = noiseMaskScrollSpeed;
		j["gpuCurlNoiseEnabled"] = gpuCurlNoiseEnabled;
		j["gpuCurlNoiseFrequency"] = gpuCurlNoiseFrequency;
		j["gpuCurlNoiseOctaves"] = gpuCurlNoiseOctaves;
		j["gpuCurlNoiseRoughness"] = gpuCurlNoiseRoughness;
		j["gpuCurlNoiseLacunarity"] = gpuCurlNoiseLacunarity;
		j["gpuCurlNoiseAmplitude"] = gpuCurlNoiseAmplitude;
		j["gpuCurlNoiseOffset"] = gpuCurlNoiseOffset;
		j["gpuCurlNoiseScrollSpeed"] = gpuCurlNoiseScrollSpeed;
		j["isDrawEnable"]	= isDrawEnable;
		j["isComplement"]	= isComplement;
		j["randomSpinEmit"] = randomSpinEmit;
		j["fixedRandomSeed"] = fixedRandomSeed;
		j["randomSeed"] = randomSeed;
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
