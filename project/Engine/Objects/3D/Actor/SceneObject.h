#pragma once

// engine
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/objects/Transform/Transform.h>

// c++
#include <memory>
#include <optional>
#include <string>
#include <vector>

// externals
#include <externals/nlohmann/json.hpp>

enum class ObjectType {
	Camera,			// カメラ
	Light,			// ライト
	GameObject,		// ゲームオブジェクト
	Effect,			 // パーティクルシステム
	Event,			// イベント
	None,			// なし
};

class IConfigurable; // 前方宣言

/* ========================================================================
/*		シーン上のオブジェクト
/* ===================================================================== */
class SceneObject
	: public std::enable_shared_from_this<SceneObject> {
public:
	// =======================
	// Constructors & Destructor
	// =======================
	SceneObject();
	virtual ~SceneObject();

	// =======================
	// Main Interface
	// =======================
	virtual void Initialize() {}
	virtual void AlwaysUpdate([[maybe_unused]] float dt) {}; //< 常時更新
	virtual void Update([[maybe_unused]] float dt) {};		 //< ランタイム時更新

	virtual void Draw([[maybe_unused]] ID3D12GraphicsCommandList* cmdList) {}
	virtual void ShowGui();

	virtual bool Save() const;
	virtual bool Load();

	virtual void ApplyDerivedConfigFromJson([[maybe_unused]] const nlohmann::json& root,
											[[maybe_unused]] const nlohmann::json* derived) {}
	virtual void ExtractDerivedConfigToJson([[maybe_unused]] nlohmann::json& root,
											[[maybe_unused]] nlohmann::json& derived) const {}

	// =======================
	// Bounding Volume
	// =======================
	virtual AABB GetWorldAABB() const { return FallbackAABBFromTransform(); }

	// トランスフォームから簡易AABBを計算
	AABB FallbackAABBFromTransform() const;

	// =======================
	// Config I/O virtuals
	// =======================
	virtual std::string GetObjectTypeName() const;

	virtual void SetName(const std::string& name, std::optional<ObjectType> type);

	void SetConfigPath(const std::string& path) { configPath_ = path; }

	// =======================
	// Serialization and Config Interface
	// =======================
	virtual bool IsSerializable() const { return true; }

	virtual bool HasConfigInterface() const;

	// =======================
	// Accessors
	// =======================
	const std::vector<std::shared_ptr<SceneObject>>& GetChildren() const { return children_; }

	const WorldTransform&		 GetWorldTransform() const { return worldTransform_; }
	WorldTransform&				 GetWorldTransform() { return worldTransform_; }
	std::shared_ptr<SceneObject> GetParent() const { return parent_.lock(); }
	virtual std::string_view	 GetTypeName() const { return "SceneObject"; }
	ObjectType					 GetObjectType() const { return objectType_; }
	const Guid&					 GetGuid() const { return id_; }
	const std::string&			 GetName() const { return name_; }
	const std::string&			 GetConfigPath() const;
	bool						 IsEnableRaycast() const { return isEnableRaycast_; }
	bool						 IsDrawEnable() const { return isDrawEnable_; }

	void		 SetGuid(const Guid& g) { id_ = g; }
	virtual void SetDrawEnable(bool enable) { isDrawEnable_ = enable; }
	void		 SetParent(const std::shared_ptr<SceneObject>& newParentSp, bool inheritScale = true);
	void SetEnableRaycast(bool enable) { isEnableRaycast_ = enable; }

	void UpdateWorldTransformRecursive();

	void AddChild(const std::shared_ptr<SceneObject>& child);

protected:
	// =======================
	// Identification
	// =======================
	std::string				   name_	   = "";		   //< オブジェクト名
	std::optional<std::string> configPath_ = std::nullopt; //< コンフィグファイルパス
	Guid					   id_;						   //< 識別子
	Guid					   parentId_;				   //< 親識別子
	ObjectType				   objectType_ = ObjectType::None;

	// =======================
	// Transform & Hierarchy
	// =======================
	WorldTransform							  worldTransform_; //< ワールドトランスフォーム
	std::weak_ptr<SceneObject>				  parent_;		   //< 親オブジェクト
	std::vector<std::shared_ptr<SceneObject>> children_;	   //< 子オブジェクトリスト

	// =======================
	// State Flags
	// =======================
	bool isEnableRaycast_ = true; // レイキャスト有効/無効
	bool isDrawEnable_	  = true; // 描画有効/無効
};