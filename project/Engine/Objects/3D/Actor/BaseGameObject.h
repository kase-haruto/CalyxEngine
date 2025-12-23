#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
//* engine *//
#include <Data/Engine/Configs/Scene/Objects/BaseGameObject/BaseGameObjectConfig.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <Engine/objects/Collider/Collider.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

//* c++ *//
#include <memory>
#include <string>

/**
 * ゲームオブジェクト基底クラス
 */
class BaseGameObject
	: public SceneObject,
	  public IConfigurable {

protected:
	enum class ColliderKind {
		None,
		Box,
		Sphere,
	};

public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BaseGameObject(const std::string&		  modelName,
				   std::optional<std::string> objectName = std::nullopt);
	BaseGameObject();
	virtual ~BaseGameObject() override;

	virtual void Initialize() override {}
	void		 AlwaysUpdate(float dt) override;

	//--------- ui/gui --------------------------------------------------
	void ShowGui() override;

	/// <summary>
	/// 派生先クラスのgui
	/// </summary>
	virtual void DerivativeGui();

	//--------- Collision -----------------------------------------------

	/// <summary>
	/// 衝突した瞬間の処理
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) {}

	/// <summary>
	/// 衝突中の処理
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionStay([[maybe_unused]] Collider* other) {}

	/// <summary>
	/// 衝突終了時の処理
	/// </summary>
	/// <param name="other"></param>
	virtual void OnCollisionExit([[maybe_unused]] Collider* other) {}

	//--------- config ------------------------------------------------
	// 適用
	virtual void ApplyConfig();
	void		 ApplyConfigFromJson(const nlohmann::json& j) override;

	// 吐き出し
	virtual void ExtractConfig();
	void		 ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- accessor ------------------------------------------------
	// getter
	virtual const CalyxMath::Vector3 GetCenterPos() const;
	BillboardMode		  GetBillboardMode() const { return billboardMode_; }
	std::string_view	  GetTypeName() const override { return "BaseGameObject"; }
	const CalyxMath::Vector3		  GetWorldPosition() const { return worldTransform_.GetWorldPosition(); }
	BaseModel*			  GetModel() const { return model_.get(); }
	Collider*			  GetCollider();
	ObjectModelType		  GetModelType() const { return objectModelType_; }
	Model*				  GetStaticModel();
	CalyxAssets::AnimationModel*		  AnimationModel();
	const CalyxAssets::AnimationModel* AnimationModel() const;
	AABB				  GetWorldAABB() const;

	// setter
	void SetName(const std::string& name);
	void SetBillboardMode(BillboardMode m) { billboardMode_ = m; }
	void SetTranslate(const CalyxMath::Vector3& pos);
	void SetRotate(const CalyxMath::Quaternion& rot);
	void SetRotate(const CalyxMath::Vector3& euler);
	void SetScale(const CalyxMath::Vector3& scale);
	void SetDrawEnable(bool isDrawEnable) override;
	void SetColor(const CalyxMath::Vector4& color);
	void SetCollider(std::unique_ptr<Collider> collider);
	void SetTexture(const std::string& texName);
	void SetUvScale(const CalyxMath::Vector2& scale) { model_->uvTransform.scale = scale; }
	void SetBlendMode(BlendMode mode) { model_->SetBlendMode(mode); }
	void SetLightingMode(LightingMode mode) { model_->SetLightingMode(mode); }

	//--------- save / load ------------------------------------------------
	bool Save() const override;
	bool Load() override;

protected:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	void InitializeCollider(ColliderKind kind);

protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	std::unique_ptr<BaseModel>		model_			= nullptr; // 描画用モデル
	std::unique_ptr<CalyxAssets::AnimationModel> animationModel_ = nullptr; // アニメーションモデル

protected:
	//===================================================================*/
	//                    protected variables
	//===================================================================*/
	ObjectModelType objectModelType_ = ModelType_Static;

	std::unique_ptr<Collider> collider_ = nullptr;
	ColliderKind			  currentColliderKind_ = ColliderKind::None;  //< コライダーの種類
	BillboardMode			  billboardMode_	   = BillboardMode::None; //< ビルボードモード
protected:
	//===================================================================*/
	//                    config
	//===================================================================*/
	ConfigurableObject<BaseGameObjectConfig> config_;
	const std::string configRoot_ = "BaseGameObject/";
};