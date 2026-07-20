#pragma once

#include <Engine/Foundation/Curve/ParticleCurve.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <string>

namespace CalyxEngine {

	enum class TrailFacingMode : uint8_t { CameraFacing, LocalAxis, WorldAxis, Cross };
	enum class TrailUVMode : uint8_t { Distance, Stretch };
	enum class TrailGeometryMode : uint8_t { Ribbon, MeshExtrusion, MeshInstances };

	/*------------------------------------------------------------
	    TrailのTexture、Noise、Dissolve、発光設定を保持する。
	    GPUリソースやRuntimeの経過時間は管理しない。
	------------------------------------------------------------*/
	struct TrailMaterialConfig {
		Vector4 color{1.0f,1.0f,1.0f,1.0f};
		Vector2 baseTiling{1.0f,1.0f};
		Vector2 baseScrollSpeed{};
		Vector2 noiseTiling{1.0f,1.0f};
		Vector2 noiseScrollSpeed{};
		float noiseStrength = 1.0f;
		float distortionStrength = 0.0f;
		bool dissolveEnabled = false;
		float dissolveStart = 0.0f;
		float dissolveEnd = 1.0f;
		float dissolveSoftness = 0.1f;
		float dissolveEdgeWidth = 0.0f;
		Vector3 dissolveEdgeColor{1.0f,1.0f,1.0f};
		float dissolveEdgeEmissive = 0.0f;
		float headFade = 0.0f;
		float tailFade = 0.2f;
		Vector3 emissiveColor{1.0f,1.0f,1.0f};
		float emissiveIntensity = 0.0f;
		float alphaClipThreshold = 0.0f;
	};

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TrailMaterialConfig,
		color,baseTiling,baseScrollSpeed,noiseTiling,noiseScrollSpeed,
		noiseStrength,distortionStrength,dissolveEnabled,dissolveStart,dissolveEnd,
		dissolveSoftness,dissolveEdgeWidth,dissolveEdgeColor,dissolveEdgeEmissive,
		headFade,tailFade,emissiveColor,emissiveIntensity,alphaClipThreshold)

	/*------------------------------------------------------------
	    Emitterに付随する連続Ribbon Trailの保存設定。
	    TrailPointやDynamic Bufferはシリアライズしない。
	------------------------------------------------------------*/
	struct TrailSettingsConfig {
		bool enabled = false;
		float lifetime = 0.4f;
		float baseWidth = 0.25f;
		float minSampleDistance = 0.1f;
		float maxSampleInterval = 0.05f;
		uint32_t maxPointCount = 64;
		float uvTiling = 1.0f;
		TrailUVMode uvMode = TrailUVMode::Distance;
		TrailFacingMode facingMode = TrailFacingMode::CameraFacing;
		TrailGeometryMode geometryMode = TrailGeometryMode::Ribbon;
		BlendMode blendMode = BlendMode::ADD;
		bool useSpline = false;
		uint32_t splineSubdivision = 2;
		Vector3 localAxis{1.0f,0.0f,0.0f};
		Vector3 worldAxis{0.0f,1.0f,0.0f};
		Guid geometryModelGuid{Guid::Empty()};
		std::string geometryModelPath;
		Vector3 geometryScale{1.0f,1.0f,1.0f};
		bool closeCrossSection = true;
		bool alignInstancesToTangent = true;
		Guid baseTextureGuid{Guid::Empty()};
		Guid noiseTextureGuid{Guid::Empty()};
		std::string baseTexturePath{"Textures/white1x1.dds"};
		std::string noiseTexturePath;
		ColorGradient colorOverLifetime{};
		FloatCurve alphaOverLifetime{};
		FloatCurve widthOverLifetime{};
		FloatCurve emissiveOverLifetime{};
		TrailMaterialConfig material{};
	};

	inline void to_json(nlohmann::json& j,const TrailSettingsConfig& c) {
		j = {{"enabled",c.enabled},{"lifetime",c.lifetime},{"baseWidth",c.baseWidth},
			{"minSampleDistance",c.minSampleDistance},{"maxSampleInterval",c.maxSampleInterval},
			{"maxPointCount",c.maxPointCount},{"uvTiling",c.uvTiling},{"uvMode",c.uvMode},
			{"facingMode",c.facingMode},{"geometryMode",c.geometryMode},{"blendMode",c.blendMode},{"useSpline",c.useSpline},
			{"splineSubdivision",c.splineSubdivision},{"localAxis",c.localAxis},{"worldAxis",c.worldAxis},
			{"geometryModelGuid",c.geometryModelGuid},{"geometryModelPath",c.geometryModelPath},
			{"geometryScale",c.geometryScale},{"closeCrossSection",c.closeCrossSection},{"alignInstancesToTangent",c.alignInstancesToTangent},
			{"baseTextureGuid",c.baseTextureGuid},{"noiseTextureGuid",c.noiseTextureGuid},
			{"baseTexturePath",c.baseTexturePath},{"noiseTexturePath",c.noiseTexturePath},
			{"colorOverLifetime",c.colorOverLifetime},{"alphaOverLifetime",c.alphaOverLifetime},
			{"widthOverLifetime",c.widthOverLifetime},{"emissiveOverLifetime",c.emissiveOverLifetime},
			{"material",c.material}};
	}

	inline void from_json(const nlohmann::json& j,TrailSettingsConfig& c) {
		c.enabled = j.value("enabled",false);
		c.lifetime = (std::max)(j.value("lifetime",0.4f),0.001f);
		c.baseWidth = (std::max)(j.value("baseWidth",0.25f),0.0f);
		c.minSampleDistance = (std::max)(j.value("minSampleDistance",0.1f),0.001f);
		c.maxSampleInterval = (std::max)(j.value("maxSampleInterval",0.05f),0.001f);
		c.maxPointCount = std::clamp(j.value("maxPointCount",64u),2u,4096u);
		c.uvTiling = j.value("uvTiling",1.0f);
		c.uvMode = j.value("uvMode",TrailUVMode::Distance);
		c.facingMode = j.value("facingMode",TrailFacingMode::CameraFacing);
		c.geometryMode = j.value("geometryMode",TrailGeometryMode::Ribbon);
		c.blendMode = j.value("blendMode",BlendMode::ADD);
		c.useSpline = j.value("useSpline",false);
		c.splineSubdivision = std::clamp(j.value("splineSubdivision",2u),1u,8u);
		c.localAxis = j.value("localAxis",Vector3{1,0,0});
		c.worldAxis = j.value("worldAxis",Vector3{0,1,0});
		c.geometryModelGuid = j.value("geometryModelGuid",Guid::Empty());
		c.geometryModelPath = j.value("geometryModelPath",std::string{});
		c.geometryScale = j.value("geometryScale",Vector3{1,1,1});
		c.closeCrossSection = j.value("closeCrossSection",true);
		c.alignInstancesToTangent = j.value("alignInstancesToTangent",true);
		c.baseTextureGuid = j.value("baseTextureGuid",Guid::Empty());
		c.noiseTextureGuid = j.value("noiseTextureGuid",Guid::Empty());
		c.baseTexturePath = j.value("baseTexturePath",std::string{"Textures/white1x1.dds"});
		c.noiseTexturePath = j.value("noiseTexturePath",std::string{});
		c.colorOverLifetime = j.value("colorOverLifetime",ColorGradient{});
		c.alphaOverLifetime = j.value("alphaOverLifetime",FloatCurve{});
		c.widthOverLifetime = j.value("widthOverLifetime",FloatCurve{});
		c.emissiveOverLifetime = j.value("emissiveOverLifetime",FloatCurve{});
		c.material = j.value("material",TrailMaterialConfig{});
	}
}
