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
	Camera,		// カメラ
	Light,		// ライト
	GameObject, // ゲームオブジェクト
	Effect,		// パーティクルシステム
	Event,		// イベント
	None,		// なし
};

class IConfigurable; // 前方宣言

/*-----------------------------------------------------------------------------------------
 * SceneObject
 * - シーンオブジェクト基底クラス
 * - シーン上に配置可能な全オブジェクトの基盤となるクラス
 *---------------------------------------------------------------------------------------*/
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
	/**
	 * \brief オブジェクト初期化
	 */
	virtual void Initialize() {}
	/**
	 * \brief オブジェクト更新(常時更新)
	 */
	virtual void AlwaysUpdate([[maybe_unused]] float dt) {}
	/**
	 * \brief オブジェクト更新(ランタイム時のみ)
	 */
	virtual void Update([[maybe_unused]] float dt) {}
	/**
	 * \brief オブジェクト描画
	 */
	virtual void Draw([[maybe_unused]] ID3D12GraphicsCommandList* cmdList) {}
	/**
	 * \brief デバッグGUI表示
	 */
	virtual void ShowGui();
	/**
	 * \brief オブジェクト破棄
	 */
	virtual void Destroy();

	// =======================
	// configs
	// =======================
	/**
	 * \brief オブジェクト設定保存
	 */
	virtual bool Save() const;
	/**
	 * \brief オブジェクト設定読み込み
	 */
	virtual bool Load();
	/**
	 * \brief JSONから派生クラスの設定を適用
	 */
	virtual void ApplyDerivedConfigFromJson([[maybe_unused]] const nlohmann::json& root,
											[[maybe_unused]] const nlohmann::json* derived) {}

	/**
	 * \brief 派生クラスの設定をJSONに抽出
	 */
	virtual void ExtractDerivedConfigToJson([[maybe_unused]] nlohmann::json& root,
											[[maybe_unused]] nlohmann::json& derived) const {}

	// =======================
	// Config I/O virtuals
	// =======================
	virtual std::string GetObjectTypeName() const;
	virtual bool		IsSerializable() const { return !isTransient_; }
	virtual bool		HasConfigInterface() const;
	virtual AABB		GetWorldAABB() const { return FallbackAABBFromTransform(); }
	AABB				FallbackAABBFromTransform() const;
	virtual void		SetName(const std::string& name, std::optional<ObjectType> type);
	void				SetConfigPath(const std::string& path) { configPath_ = path; }

	// =======================
	// Serialization and Config Interface
	// =======================

	// =======================
	// Accessors
	// =======================
	const std::vector<std::shared_ptr<SceneObject>>& GetChildren() const { return children_; }
	const WorldTransform&							 GetWorldTransform() const { return worldTransform_; }
	WorldTransform&									 GetWorldTransform() { return worldTransform_; }
	std::shared_ptr<SceneObject>					 GetParent() const { return parent_.lock(); }
	virtual std::string_view						 GetTypeName() const { return "SceneObject"; }
	ObjectType										 GetObjectType() const { return objectType_; }
	const Guid&										 GetGuid() const { return id_; }
	const std::string&								 GetName() const { return name_; }
	const std::string&								 GetConfigPath() const;
	bool											 IsEnableRaycast() const { return isEnableRaycast_; }
	bool											 IsDrawEnable() const { return isDrawEnable_; }
	bool											 IsPickable() const { return isEnablePicking_; }
	bool											 IsTransient() const { return isTransient_; }
	bool											 IsCastShadow() const { return isCastShadow_; }
	uint32_t										 GetPickingID() const { return pickingID_; }

	void		 SetGuid(const Guid& g) { id_ = g; }
	virtual void SetDrawEnable(bool enable) { isDrawEnable_ = enable; }
	void		 SetEnablePicking(bool enable) { isEnablePicking_ = enable; }
	void		 SetTransient(bool enable) { isTransient_ = enable; }
	void		 SetCastShadow(bool enable) { isCastShadow_ = enable; }
	void		 SetParent(const std::shared_ptr<SceneObject>& newParentSp, bool inheritScale = true);
	void		 SetEnableRaycast(bool enable) { isEnableRaycast_ = enable; }
	void		 SetPickingID(uint32_t id) { pickingID_ = id; }

	void AddChild(const std::shared_ptr<SceneObject>& child);

protected:
	/**
	 * \brief 自分と子オブジェクトを再帰的に破棄する
	 */
	void DestroyRecursive();

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
	bool	 isEnableRaycast_ = false; // レイキャスト有効/無効
	bool	 isDrawEnable_	  = true;  // 描画有効/無効
	bool	 isEnablePicking_ = true;  // ピッキング有効
	bool	 isTransient_	  = false; // 一時的（保存・階層除外）
	bool 	isCastShadow_	  = true;  // 影を落とすか
	uint32_t pickingID_		  = 0;
};