#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Data/Engine/Configs/Scene/Objects/SceneObject/SceneObjectConfig.h>
#include <string>
#include <externals/nlohmann/json.hpp>

struct DirectionalLightConfig : public SceneObjectConfig {
	CalyxEngine::Vector4 color;     //< 光の色
	CalyxEngine::Vector3 direction; //< 光の方向
	float   intensity; //< 光の強度
};

inline void to_json(nlohmann::json& j,const DirectionalLightConfig& c) {
	j = nlohmann::json{
			{"guid",c.guid},
			{"parentGuid",c.parentGuid},
			{"objectType",c.objectType},
			{"name",c.name},
			{"transform",c.transform},
			{"color",c.color},
			{"direction",c.direction},
			{"intensity",c.intensity}
		};
}

inline void from_json(const nlohmann::json& j,DirectionalLightConfig& c) {
	c.guid       = j.value("guid",Guid::Empty());
	c.parentGuid = j.value("parentGuid",Guid::Empty());
	c.objectType = j.value("objectType",(int)ObjectType::Light);
	c.name       = j.value("name","DirectionalLight");

	// transform は存在チェックしてから読む
	if(j.contains("transform")) { j.at("transform").get_to(c.transform); } else {
		c.transform = WorldTransformConfig(); // デフォルト構築
	}

	c.color     = j.value("color",CalyxEngine::Vector4(1,1,1,1));
	c.direction = j.value("direction",CalyxEngine::Vector3(0,-1,0));
	c.intensity = j.value("intensity",1.0f);
}