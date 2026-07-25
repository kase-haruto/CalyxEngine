#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
/* ========================================================================
/* include space
/* ===================================================================== */
//* engine *//
#include <Data/Engine/Configs/Scene/Objects/BaseGameObject/BaseGameObjectConfig.h>
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Foundation/Reflection/CalyxReflection.h>
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <Engine/objects/Collider/Collider.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>
#include <Engine/Physics/PhysicsBody.h>

//* c++ *//
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/*-----------------------------------------------------------------------------------------
 * BaseGameObject
 * - Transform、モデル参照、Collider、描画設定などの共通機能を管理する
 * - シーン保存対象、Editor配置一覧、Prefab対象かどうかは管理しない
 *---------------------------------------------------------------------------------------*/
class CALYX_API BaseGameObject
	: public SceneObject,
	  public IConfigurable {

protected:
	/*-----------------------------------------------------------------------------------------
	 * ColliderKind
	 * - BaseGameObjectが所有するColliderの具体的な形状を識別する列挙型
	 * - Config保存値からRuntime Colliderを再構築する際に使用
	 *---------------------------------------------------------------------------------------*/
	enum class ColliderKind {
		None,
		Box,
		Sphere,
		Capsule,
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
	/**
	 * \brief コライダーのデバッグ表示
	 */
	void		 DrawCollider();

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
	virtual const CalyxEngine::Vector3 GetCenterPos() const;
	/**
	 * \brief ビルボードモードを取得
	 * \return ビルボードモード
	 */
	BillboardMode		  GetBillboardMode() const { return billboardMode_; }
	/**
	 * \brief タイプ名を取得
	 * \return タイプ名
	 */
	std::string_view	  GetObjectClassName() const override { return "BaseGameObject"; }
	/**
	 * \brief ワールド座標を取得
	 * \return ワールド座標
	 */
	const CalyxEngine::Vector3		  GetWorldPosition() const { return worldTransform_.GetWorldPosition(); }
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
	 * \brief 物理応答設定を取得
	 * \return 物理応答設定
	 */
	PhysicsBody&		  GetPhysicsBody() { return physicsBody_; }
	const PhysicsBody&	  GetPhysicsBody() const { return physicsBody_; }
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
	CalyxEngine::AnimationModel*		  AnimationModel();
	/**
	 * \brief アニメーションモデルを取得 (const)
	 * \return アニメーションモデル
	 */
	const CalyxEngine::AnimationModel* AnimationModel() const;
	/**
	 * \brief ワールド座標系AABBを取得
	 * \return AABB
	 */
	AABB				  GetWorldAABB() const;
	/**
	 * \brief 描画専用オフセットを反映したTransformを取得
	 * \return 描画用Transform
	 */
	WorldTransform		  GetRenderWorldTransform() const;
	/**
	 * \brief 描画モデルだけに適用するローカルオフセットを取得
	 * \return 描画オフセット
	 */
	const CalyxEngine::Vector3& GetVisualOffset() const { return visualOffset_; }

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
	void SetTranslate(const CalyxEngine::Vector3& pos);
	/**
	 * \brief 回転を設定 (クォータニオン)
	 * \param rot 回転
	 */
	void SetRotate(const CalyxEngine::Quaternion& rot);
	/**
	 * \brief 回転を設定 (オイラー角)
	 * \param euler 回転
	 */
	void SetRotate(const CalyxEngine::Vector3& euler);
	/**
	 * \brief スケールを設定
	 * \param scale スケール
	 */
	void SetScale(const CalyxEngine::Vector3& scale);
	/**
	 * \brief 描画モデルだけに適用するローカルオフセットを設定
	 * \param offset 描画オフセット
	 */
	void SetVisualOffset(const CalyxEngine::Vector3& offset) { visualOffset_ = offset; }
	/**
	 * \brief 描画の有効/無効を設定
	 * \param isDrawEnable 有効か
	 */
	void SetDrawEnable(bool isDrawEnable) override;
	/**
	 * \brief 色を設定
	 * \param color 色
	 */
	void SetColor(const CalyxEngine::Vector4& color);
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
	bool SetModelByGuid(const Guid& guid);
	bool SetModelFileNameForEditor(const std::string& modelName);
	const std::string& GetModelFileName() const { return config_.GetConfig().modelConfig.modelName; }
	bool RegisterAnimationClip(int16_t animId, const std::string& animName, const std::optional<std::string>& fileName = std::nullopt);
	void PlayRegisteredAnimation(int16_t animId, float blendDuration = 0.2f);
	/**
	 * \brief UVスケールを設定
	 * \param scale スケール
	 */
	void SetUvScale(const CalyxEngine::Vector2& scale) { model_->uvTransform.scale = scale; }
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
	/** Applies a per-actor rim-light override without modifying the shared material asset. */
	void SetRimLight(const CalyxEngine::Vector4& color, float intensity, float power = 3.0f) {
		if(model_) model_->SetRimLight(color, intensity, power);
	}
	/** Restores the rim-light values from the shared material asset. */
	void ClearRimLight() {
		if(model_) model_->ClearRimLight();
	}
	bool HasRimLightOverride() const {
		return model_ && model_->HasRimLightOverride();
	}

	/**
	 * \brief ボーン親子家計を設定
	 * \param target
	 * \param boneName
	 * \param inheritScale
	 */
	void SetBoneParent(WorldTransform& target, const std::string& boneName, bool inheritScale = true);
	/**
	 * \brief ボーンの親子関係を解除
	 * \param target
	 */
	void ClearBoneParent(WorldTransform& target);
	bool HasBoneParentTarget(const WorldTransform* target) const;
	std::optional<std::string> GetBoneParentNameForTarget(const WorldTransform* target) const;
	std::vector<std::string> GetBoneNamesForEditor() const;
	bool IsSkeletonDrawEnabledForEditor() const;
	void SetSkeletonDrawEnabledForEditor(bool enabled);
	bool SelectBoneForEditor(const std::string& boneName);
	/*-----------------------------------------------------------------------------------------
	 * BoneParentBindingInfo
	 * - Editorへ公開するボーン親子付け情報の読み取り専用スナップショット
	 * - 対象Transformを所有せず、ボーン名と拡大率継承設定を保持
	 *---------------------------------------------------------------------------------------*/
	struct BoneParentBindingInfo {
		const WorldTransform* target = nullptr; //< 所有権を持たない親子付け対象Transform
		std::string boneName;                   //< 親として使用するスケルトンのボーン名
		bool inheritScale = true;               //< ボーンの拡大率を対象へ継承するか
	};
	/** \brief 登録中のボーン親子付け情報を取得する \return Editor表示用の非所有参照情報 */
	std::vector<BoneParentBindingInfo> GetBoneParentBindings() const;


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
	bool SetModelFromFileName(const std::string& modelName);

	/**
	 * \brief ボーン親子関係の更新
	 */
	void UpdateBoneParents();

protected:
	/*-----------------------------------------------------------------------------------------
	 * BoneParentTransform
	 * - ボーン行列をWorldTransformの親として公開する内部Transform
	 * - GPUリソースを所有せず、BaseGameObjectが計算した行列を保持
	 *---------------------------------------------------------------------------------------*/
	class BoneParentTransform : public BaseTransform {
	public:
		/** \brief ボーンから計算したワールド行列を設定する \param world 設定するワールド行列 */
		void SetWorldMatrix(const CalyxEngine::Matrix4x4& world);
		/** \brief 外部設定済み行列を維持するため通常更新を行わない */
		void Update() override {}
		/** \brief 外部設定済み行列を維持するためビュー付き更新を行わない \param viewProMatrix 未使用のビュープロジェクション行列 */
		void Update([[maybe_unused]] const CalyxEngine::Matrix4x4& viewProMatrix) override {}
	};

	/*-----------------------------------------------------------------------------------------
	 * BoneParentBinding
	 * - ボーンと子TransformのRuntime親子関係を保持する内部データ構造
	 * - 中継Transformを所有し、対象WorldTransformは所有しない
	 *---------------------------------------------------------------------------------------*/
	struct BoneParentBinding {
		WorldTransform* target = nullptr;                         //< 所有権を持たない親子付け対象Transform
		std::string boneName;                                    //< 親として参照するボーン名
		std::unique_ptr<BoneParentTransform> parentTransform;    //< ボーン行列を公開する中継Transform
		bool inheritScale = true;                                //< ボーンの拡大率を継承するか
	};

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	std::unique_ptr<BaseModel>		model_			= nullptr; //< 描画用モデル
	std::unique_ptr<CalyxEngine::AnimationModel> animationModel_ = nullptr; //< アニメーションモデル

	ObjectModelType objectModelType_ = ModelType_Static; //< モデルタイプ

	std::unique_ptr<Collider> collider_ = nullptr; //< コライダー
	PhysicsBody			  physicsBody_; //< 物理応答設定
	ColliderKind			  currentColliderKind_ = ColliderKind::None;  //< コライダーの種類
	BillboardMode			  billboardMode_	   = BillboardMode::None; //< ビルボードモード
	CalyxEngine::Vector3	  visualOffset_	   = {0.0f, 0.0f, 0.0f}; //< 描画モデルだけに適用するローカルオフセット

	ConfigurableObject<BaseGameObjectConfig> config_; //< コンフィグ管理
	const std::string configRoot_ = "BaseGameObject/"; //< コンフィグルートパス
	std::vector<BoneParentBinding> boneParentBindings_; //< 所有するボーン親子付けのRuntime情報
};
