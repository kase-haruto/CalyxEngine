#pragma once

// engine
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>
#include <Engine/Objects/2D/Animation/TransformKeyframeAnimation2d.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/objects/Transform/Transform.h>

// c++
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// externals
#include <externals/nlohmann/json.hpp>

enum class ObjectType {
	Camera,		// カメラ
	Light,		// ライト
	GameObject, // ゲームオブジェクト
	Object2D,   // 2Dオブジェクト
	Effect,		// パーティクルシステム
	Event,		// イベント
	None,		// なし
};

class IConfigurable; // 前方宣言
namespace CalyxEngine {
	class SerializableObject;
}

struct OutlineSettings {
	bool				   enabled	 = false;
	float				   thickness = 0.035f;
	CalyxEngine::Vector4 color	 = {0.02f, 0.02f, 0.025f, 1.0f};
};

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
	virtual void RemapSceneObjectReferences([[maybe_unused]] const std::unordered_map<Guid, Guid>& guidMap) {}
	void BeginSerializableParamCapture(const nlohmann::json* overrides);
	void EndSerializableParamCapture();
	void AdoptPendingSerializableParamCapture(const nlohmann::json* overrides);
	void ExtractSerializableParamsToJson(nlohmann::json& j) const;

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
	CalyxEngine::TransformKeyframeAnimation2d&		 Get2DTransformAnimation() { return transformAnimation2d_; }
	const CalyxEngine::TransformKeyframeAnimation2d& Get2DTransformAnimation() const { return transformAnimation2d_; }
	std::shared_ptr<SceneObject>					 GetParent() const { return parent_.lock(); }
	virtual std::string_view						 GetObjectClassName() const { return "SceneObject"; }
	ObjectType										 GetObjectType() const { return objectType_; }
	const Guid&										 GetGuid() const { return id_; }
	const std::string&								 GetName() const { return name_; }
	std::string										 GetDisplayName() const;
	const std::string&								 GetConfigPath() const;
	bool											 IsEnableRaycast() const { return isEnableRaycast_; }
	bool											 IsDrawEnable() const { return isDrawEnable_; }
	bool											 IsPickable() const { return isEnablePicking_; }
	bool											 IsTransient() const { return isTransient_; }
	bool											 IsCastShadow() const { return isCastShadow_; }
	bool											 IsOutlineEnabled() const { return outlineSettings_.enabled; }
	const OutlineSettings&							 GetOutlineSettings() const { return outlineSettings_; }
	uint32_t										 GetPickingID() const { return pickingID_; }
	const Guid&										 GetPrefabAssetGuid() const { return prefabAssetGuid_; }
	const Guid&										 GetPrefabSourceGuid() const { return prefabSourceGuid_; }
	bool											 IsPrefabInstanceObject() const { return prefabAssetGuid_.isValid() && prefabSourceGuid_.isValid(); }

	void		 SetGuid(const Guid& g) { id_ = g; }
	void		 SetDuplicateNameIndex(uint32_t index) { duplicateNameIndex_ = index; }
	void		 SetPrefabLink(const Guid& assetGuid, const Guid& sourceGuid) {
		prefabAssetGuid_ = assetGuid;
		prefabSourceGuid_ = sourceGuid;
	}
	void		 ClearPrefabLink() {
		prefabAssetGuid_ = Guid{};
		prefabSourceGuid_ = Guid{};
	}
	virtual void SetDrawEnable(bool enable) { isDrawEnable_ = enable; }
	void		 SetEnablePicking(bool enable) { isEnablePicking_ = enable; }
	void		 SetTransient(bool enable) { isTransient_ = enable; }
	void		 SetCastShadow(bool enable) { isCastShadow_ = enable; }
	void		 SetOutlineEnabled(bool enable) { outlineSettings_.enabled = enable; }
	void		 SetOutlineThickness(float thickness) { outlineSettings_.thickness = thickness; }
	void		 SetOutlineColor(const CalyxEngine::Vector4& color) { outlineSettings_.color = color; }
	void		 SetParent(const std::shared_ptr<SceneObject>& newParentSp, bool inheritScale = true);
	void		 SetEnableRaycast(bool enable) { isEnableRaycast_ = enable; }
	void		 SetPickingID(uint32_t id) { pickingID_ = id; }
	std::vector<CalyxEngine::SerializableObject*>& SerializableParamObjectsMutable() {
		return serializableParamObjects_;
	}

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
	Guid					   prefabAssetGuid_;		   //< 元PrefabアセットのGUID
	Guid					   prefabSourceGuid_;		   //< Prefab内の元オブジェクトGUID
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
	OutlineSettings outlineSettings_{};
	uint32_t pickingID_		  = 0;
	uint32_t duplicateNameIndex_ = 0; //< 表示用の同名識別番号。保存名には含めない
	CalyxEngine::TransformKeyframeAnimation2d transformAnimation2d_;
	std::vector<CalyxEngine::SerializableObject*> serializableParamObjects_;
};
