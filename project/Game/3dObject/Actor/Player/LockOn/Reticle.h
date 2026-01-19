#pragma once
#include "Engine/Objects/2D/Object2d/SpriteObject2d.h"
#include "ReticleMover.h"
#include "Engine/objects/Transform/Transform.h"

/*-------------------------------------------------------------
 *	Reticle
 *	- 照準クラス
 *	- プレイヤーのロックオン照準を管理する
 *-----------------------------------------------------------*/
class Reticle {
public:
	//=============================================================*/
	// public method
	//=============================================================*/
	/** \brief コンストラクタ・デストラクタ */
	Reticle();
	~Reticle();

	/**
	 * \brief 初期化処理
	 */
	void Initialize();
	/**
	 * \brief 更新処理
	 */
	void Update(float dt);
	/**
	 * \brief 描画処理
	 * \param renderer レンダラー
	 */
	void Draw(class SpriteRenderer* renderer) const;
	/**
	 * \brief デバッグui
	 */
	void ShowGui();
	/**
	 * \brief レティクルの使用者を親として計算するため、親を設定
	 * \param transform
	 */
	void SetParent(WorldTransform* transform);

	/**
	 * \brief ターゲットリストのセット
	 * \param list 敵リスト
	 */
	void SetEnemyList(const std::list<std::shared_ptr<class Enemy>>& list);

	// accessor -----------------------------------------------------
	const CalyxMath::Vector2& GetPosition() const;
	CalyxMath::Vector3        GetPosition3D() const;

private:
	//=============================================================*/
	// public method
	//=============================================================*/
	/**
	 * \brief レティクル移動反映
	 */
	void ApplyMove(float dt);

private:
	//=============================================================*/
	// private method
	//=============================================================*/
	std::unique_ptr<Calyx2D::SpriteObject2d> reticleSprite_; //< 照準スプライト
	ReticleMover                             mover_;         //< 照準移動クラス
	WorldTransform                           transform_;
	std::list<std::weak_ptr<class Enemy>>    targets_;       //< ロックオン対象候補

	struct ReticleParam
		: public CalyxEngine::SerializableObject {
		ReticleParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float speed  = 100.0f;  //< レティクル移動速度
		float posFar = 1000.0f; //< レティクルの遠距離Z値

		float assistRadiusPx = 180.0f; //< アシスト有効半径(px)
		float assistStrength = 0.15f;  //< アシスト強度(0..1)

		struct SpriteParam {
			CalyxMath::Vector2 anchorPoint = {0.5f,0.5f}; //< レティクルアンカーポイント
			CalyxMath::Vector2 scale       = {64.0f,64.0f};
		}spriteParam_;

	} param_;

	std::string reticleTexturePath_ = "Textures/reticle.png"; //< 照準テクスチャパス
};