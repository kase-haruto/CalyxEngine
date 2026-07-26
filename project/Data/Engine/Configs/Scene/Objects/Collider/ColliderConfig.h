#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <Engine/Foundation/Math/Vector3.h>
#include <externals/nlohmann/json.hpp>

#include <cstdint>

/*-----------------------------------------------------------------------------------------
 * ColliderConfig
 * - シーンとPrefabへ保存するコライダー設定を保持するデータ構造
 * - 有効状態、レイヤー、形状ごとの寸法とローカル補正を管理
 * - Runtimeのコライダー本体や衝突相手への参照は管理しない
 *---------------------------------------------------------------------------------------*/
/**
 * @brief ColliderConfigに関するデータを保持する構造体です。
 */
struct ColliderConfig {
	//========================= variable =========================
	bool			   isCollisionEnabled = true;				//< コリジョン有効フラグ
	bool			   isDraw			  = true;				//< 描画有効フラグ
	bool			   isTrigger		  = false;				//< 物理応答を行わない検出専用フラグ
	// 表示名ではなくIDを保存するため、LayerをリネームしてもSceneやPrefabの更新は不要。
	// JSONとの変換を安定させるためuint32_tで保持し、Collider適用時に0～31へ検証する。
	uint32_t		   layerId			  = 0;					//< Collision Layer ID
	CalyxEngine::Vector3 offset			  = {0.0f, 0.0f, 0.0f}; //< オフセット
	CalyxEngine::Vector3 rotate			  = {0.0f, 0.0f, 0.0f}; //< 回転 (Euler)
	CalyxEngine::Vector3 size				  = {1.0f, 1.0f, 1.0f}; //< サイズ (Box)
	float			   radius			  = 1.0f;				//< 半径 (Sphere)
	float			   capsuleRadius	  = 0.5f;				//< 半径 (Capsule)
	float			   capsuleHeight	  = 2.0f;				//< 全体高さ (Capsule)
};

/**
 * \brief コライダー設定を互換性のあるJSON形式へ変換する
 * \param j 変換結果を書き込むJSON
 * \param c 保存するコライダー設定
 */
inline void to_json(nlohmann::json& j, const ColliderConfig& c) {
	// 旧分類enumやColliderごとの対象Maskは出力せず、Layer IDだけを永続化する。
	j = nlohmann::json{
		{"isCollisionEnabled", c.isCollisionEnabled},
		{"isDraw", c.isDraw},
		{"isTrigger", c.isTrigger},
		{"layerId", c.layerId},
		{"offset", c.offset},
		{"rotate", c.rotate},
		{"size", c.size},
		{"radius", c.radius},
		{"capsuleRadius", c.capsuleRadius},
		{"capsuleHeight", c.capsuleHeight}};
}

/**
 * \brief JSONからコライダー設定を復元する
 * \param j 読み込むJSON
 * \param c 復元先のコライダー設定
 */
inline void from_json(const nlohmann::json& j, ColliderConfig& c) {
	c.isCollisionEnabled = j.value("isCollisionEnabled", true);
	c.isDraw			 = j.value("isDraw", true);
	c.isTrigger			 = j.value("isTrigger", false);
	// Layer IDのない旧データは汎用のDefault Layerとして安全に読み込む。
	// 範囲検証はCollider::ApplyConfigでも行い、外部編集されたJSONに対しても安全性を保つ。
	c.layerId			 = j.value("layerId", 0u);

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
