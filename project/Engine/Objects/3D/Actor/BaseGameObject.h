#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
//* engine *//
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>
#include <Data/Engine/Configs/Scene/Objects/BaseGameObject/BaseGameObjectConfig.h>
#include <Engine/objects/Collider/Collider.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>

//* c++ lib *//
#include <memory>
#include <string>

class BaseGameObject
	: public SceneObject,
	public IConfigurable{

	enum class ColliderKind{
		None,
		Box,
		Sphere,
	};

public:
	//===================================================================*/
	//                    public methods
	//===================================================================*/
	BaseGameObject(const std::string& modelName,
				   std::optional<std::string> objectName = std::nullopt);
	BaseGameObject();
	virtual ~BaseGameObject()override;

	virtual void Initialize()override{}
	void AlwaysUpdate(float dt)override;

	//--------- ui/gui --------------------------------------------------
	void ShowGui()override;
	virtual void DerivativeGui();

	//--------- Collision -----------------------------------------------
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other){}
	virtual void OnCollisionStay([[maybe_unused]] Collider* other){}
	virtual void OnCollisionExit([[maybe_unused]] Collider* other){}

	//--------- config ------------------------------------------------
	virtual void ApplyConfig();
	virtual void ExtractConfig();
	void ApplyConfigFromJson(const nlohmann::json& j) override;
	void ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- accessor ------------------------------------------------
	void SetBillboardMode(BillboardMode m){ billboardMode_ = m; }
	BillboardMode GetBillboardMode() const { return billboardMode_; }
	void SetName(const std::string& name);
	std::string_view GetTypeName() const override{ return "BaseGameObject"; }
	void SetTranslate(const Vector3& pos);
	void SetScale(const Vector3& scale);
	void SetDrawEnable(bool isDrawEnable)override;
	virtual const Vector3 GetCenterPos()const;
	void SetColor(const Vector4& color);
	Vector3 GetWorldPosition()const{ return worldTransform_.GetWorldPosition(); }
	BaseModel* GetModel() const{ return model_.get(); }
	void SetCollider(std::unique_ptr<Collider> collider);
	Collider* GetCollider();
	void SetTexture(const std::string& texName);
	void SetUvScale(const Vector2& scale){ model_->uvTransform.scale = scale; }
	void SetBlendMode(BlendMode mode) { model_->SetBlendMode(mode); }
	void SetLightingMode(LightingMode mode) { model_->SetLightingMode(mode); }

	ObjectModelType GetModelType() const{ return objectModelType_; }

	Model* GetStaticModel();
	AnimationModel* GetAnimationModel();
	const AnimationModel* GetAnimationModel() const;
	AABB GetWorldAABB() const;
private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	void SwitchCollider(ColliderKind kind, bool isCollisionEnubled = true);

protected:
	//===================================================================*/
	//                    protected methods
	//===================================================================*/
	std::unique_ptr<BaseModel> model_ = nullptr;					// 描画用モデル
	std::unique_ptr<AnimationModel> animationModel_ = nullptr;		// アニメーションモデル

protected:
	//===================================================================*/
	//                    protected variables
	//===================================================================*/
	ObjectModelType objectModelType_ = ModelType_Static;

	std::unique_ptr<Collider> collider_;
	ColliderKind currentColliderKind_ = ColliderKind::None;	//< コライダーの種類
	BillboardMode billboardMode_ = BillboardMode::None;		//< ビルボードモード
protected:
	//===================================================================*/
	//                    config
	//===================================================================*/
	ConfigurableObject<BaseGameObjectConfig> config_;
};
