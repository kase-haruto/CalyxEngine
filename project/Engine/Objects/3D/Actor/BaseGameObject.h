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

/*-----------------------------------------------------------------------------------------
 * BaseGameObject
 * - ゲームオブジェクト基底クラス
 * - 3Dモデル、コライダー、ビルボードの設定などを統合管理する基底クラス
 *---------------------------------------------------------------------------------------*/
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
	/**
	 * \brief コンストラクタ
	 * \param modelName モデル名
	 * \param objectName オブジェクト名
	 */
	BaseGameObject(const std::string&		  modelName,
				   std::optional<std::string> objectName = std::nullopt);
	/**
	 * \brief コンストラクタ
	 */
	BaseGameObject();
	/**
	 * \brief デストラクタ
	 */
	virtual ~BaseGameObject() override;

	/**
	 * \brief 初期化
	 */
	virtual void Initialize() override {}
	/**
	 * \brief 常に実行される更新処理
	 * \param dt デルタタイム
	 */
	void		 AlwaysUpdate(float dt) override;

	//--------- ui/gui --------------------------------------------------
	/**
	 * \brief GUI表示
	 */
	void ShowGui() override;

	/**
	 * \brief パラメータ調整前のヘッダーgui
	 */
	virtual void HeaderGui();

	/**
	 * \brief 派生先クラスのgui
	 */
	virtual void DerivativeGui();

	//--------- Collision -----------------------------------------------

	/**
	 * \brief 衝突した瞬間の処理
	 * \param other 衝突相手
	 */
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) {}

	/**
	 * \brief 衝突中の処理
	 * \param other 衝突相手
	 */
	virtual void OnCollisionStay([[maybe_unused]] Collider* other) {}

	/**
	 * \brief 衝突終了時の処理
	 * \param other 衝突相手
	 */
	virtual void OnCollisionExit([[maybe_unused]] Collider* other) {}

	//--------- config ------------------------------------------------
	/**
	 * \brief コンフィグの適用
	 */
	virtual void ApplyConfig();
	/**
	 * \brief JSONからコンフィグを適用
	 * \param j JSONデータ
	 */
	void		 ApplyConfigFromJson(const nlohmann::json& j) override;

	/**
	 * \brief コンフィグの抽出
	 */
	virtual void ExtractConfig();
	/**
	 * \brief コンフィグをJSONに抽出
	 * \param j JSONデータ
	 */
	void		 ExtractConfigToJson(nlohmann::json& j) const override;

	//--------- accessor ------------------------------------------------
	// getter
	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	virtual const CalyxMath::Vector3 GetCenterPos() const;
	/**
	 * \brief ビルボードモードを取得
	 * \return ビルボードモード
	 */
	BillboardMode		  GetBillboardMode() const { return billboardMode_; }
	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view	  GetTypeName() const override { return "BaseGameObject"; }
	/**
	 * \brief ワールド座標を取得
	 * \return ワールド座標
	 */
	const CalyxMath::Vector3		  GetWorldPosition() const { return worldTransform_.GetWorldPosition(); }
	/**
	 * \brief モデルを取得
	 * \return モデル
	 */
	BaseModel*			  GetModel() const { return model_.get(); }
	/**
	 * \brief コライダーを取得
	 * \return コライダー
	 */
	Collider*			  GetCollider();
	/**
	 * \brief モデルタイプを取得
	 * \return モデルタイプ
	 */
	ObjectModelType		  GetModelType() const { return objectModelType_; }
	/**
	 * \brief スタティックモデルを取得
	 * \return モデル
	 */
	Model*				  GetStaticModel();
	/**
	 * \brief アニメーションモデルを取得
	 * \return アニメーションモデル
	 */
	CalyxAssets::AnimationModel*		  AnimationModel();
	/**
	 * \brief アニメーションモデルを取得 (const)
	 * \return アニメーションモデル
	 */
	const CalyxAssets::AnimationModel* AnimationModel() const;
	/**
	 * \brief ワールド座標系AABBを取得
	 * \return AABB
	 */
	AABB				  GetWorldAABB() const;

	// setter
	/**
	 * \brief 名前を設定
	 * \param name オブジェクト名
	 */
	void SetName(const std::string& name);
	/**
	 * \brief ビルボードモードを設定
	 * \param m モード
	 */
	void SetBillboardMode(BillboardMode m) { billboardMode_ = m; }
	/**
	 * \brief 座標を設定
	 * \param pos 座標
	 */
	void SetTranslate(const CalyxMath::Vector3& pos);
	/**
	 * \brief 回転を設定 (クォータニオン)
	 * \param rot 回転
	 */
	void SetRotate(const CalyxMath::Quaternion& rot);
	/**
	 * \brief 回転を設定 (オイラー角)
	 * \param euler 回転
	 */
	void SetRotate(const CalyxMath::Vector3& euler);
	/**
	 * \brief スケールを設定
	 * \param scale スケール
	 */
	void SetScale(const CalyxMath::Vector3& scale);
	/**
	 * \brief 描画の有効/無効を設定
	 * \param isDrawEnable 有効か
	 */
	void SetDrawEnable(bool isDrawEnable) override;
	/**
	 * \brief 色を設定
	 * \param color 色
	 */
	void SetColor(const CalyxMath::Vector4& color);
	/**
	 * \brief コライダーを設定
	 * \param collider コライダー
	 */
	void SetCollider(std::unique_ptr<Collider> collider);
	/**
	 * \brief テクスチャを設定
	 * \param texName テクスチャ名
	 */
	void SetTexture(const std::string& texName);
	/**
	 * \brief UVスケールを設定
	 * \param scale スケール
	 */
	void SetUvScale(const CalyxMath::Vector2& scale) { model_->uvTransform.scale = scale; }
	/**
	 * \brief ブレンドモードを設定
	 * \param mode モード
	 */
	void SetBlendMode(BlendMode mode) { model_->SetBlendMode(mode); }
	/**
	 * \brief ライティングモードを設定
	 * \param mode モード
	 */
	void SetLightingMode(LightingMode mode) { model_->SetLightingMode(mode); }

	//--------- save / load ------------------------------------------------
	/**
	 * \brief 保存処理
	 * \return 成功したか
	 */
	bool Save() const override;
	/**
	 * \brief 読み込み処理
	 * \return 成功したか
	 */
	bool Load() override;

protected:
	//===================================================================*/
	//                    private methods
	//===================================================================*/
	void InitializeCollider(ColliderKind kind);

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	std::unique_ptr<BaseModel>		model_			= nullptr; //< 描画用モデル
	std::unique_ptr<CalyxAssets::AnimationModel> animationModel_ = nullptr; //< アニメーションモデル

	ObjectModelType objectModelType_ = ModelType_Static; //< モデルタイプ

	std::unique_ptr<Collider> collider_ = nullptr; //< コライダー
	ColliderKind			  currentColliderKind_ = ColliderKind::None;  //< コライダーの種類
	BillboardMode			  billboardMode_	   = BillboardMode::None; //< ビルボードモード

	ConfigurableObject<BaseGameObjectConfig> config_; //< コンフィグ管理
	const std::string configRoot_ = "BaseGameObject/"; //< コンフィグルートパス
};