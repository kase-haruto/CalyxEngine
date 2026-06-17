#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

struct ColliderConfig {
	//========================= variable =========================
	bool			   isCollisionEnabled = true;				//< コリジョン有効フラグ
	bool			   isDraw			  = true;				//< 描画有効フラグ
	int				   colliderType		  = 0;					//< コリジョンの種類
	int				   targetType		  = 0;					//< 衝突相手の種類
	CalyxEngine::Vector3 offset			  = {0.0f, 0.0f, 0.0f}; //< オフセット
	CalyxEngine::Vector3 rotate			  = {0.0f, 0.0f, 0.0f}; //< 回転 (Euler)
	CalyxEngine::Vector3 size				  = {1.0f, 1.0f, 1.0f}; //< サイズ (Box)
	float			   radius			  = 1.0f;				//< 半径 (Sphere)
	float			   capsuleRadius	  = 0.5f;				//< 半径 (Capsule)
	float			   capsuleHeight	  = 2.0f;				//< 全体高さ (Capsule)
};

inline void to_json(nlohmann::json& j, const ColliderConfig& c) {
	j = nlohmann::json{
		{"isCollisionEnabled", c.isCollisionEnabled},
		{"isDraw", c.isDraw},
		{"colliderType", c.colliderType},
		{"targetType", c.targetType},
		{"offset", c.offset},
		{"rotate", c.rotate},
		{"size", c.size},
		{"radius", c.radius},
		{"capsuleRadius", c.capsuleRadius},
		{"capsuleHeight", c.capsuleHeight}};
}

inline void from_json(const nlohmann::json& j, ColliderConfig& c) {
	c.isCollisionEnabled = j.value("isCollisionEnabled", true);
	c.isDraw			 = j.value("isDraw", true);
	c.colliderType		 = j.value("colliderType", 0);
	c.targetType		 = j.value("targetType", 0);

	if(j.contains("offset")) {
		j.at("offset").get_to(c.offset);
	}
	if(j.contains("rotate")) {
		j.at("rotate").get_to(c.rotate);
	}
	if(j.contains("size")) {
		j.at("size").get_to(c.size);
	}
	if(j.contains("radius")) {
		j.at("radius").get_to(c.radius);
	}
	if(j.contains("capsuleRadius")) {
		j.at("capsuleRadius").get_to(c.capsuleRadius);
	}
	if(j.contains("capsuleHeight")) {
		j.at("capsuleHeight").get_to(c.capsuleHeight);
	}
}
