#pragma once
#include "Engine/Objects/2D/Object2d/SpriteObject2d.h"
#include "ReticleMover.h"

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

	// accessor -----------------------------------------------------
	const CalyxMath::Vector2& GetPosition() const;
	CalyxMath::Vector3 GetPosition3D() const;

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
	ReticleMover							 mover_;		 //< 照準移動クラス

	struct ReticleParam
		: public CalyxEngine::SerializableObject {
		ReticleParam();
		CalyxEngine::ParamPath GetParamPath() const override;

		float speed   = 100.0f;   //< レティクル移動速度
		float posFar = 1000.0f; //< レティクルの遠距離Z値
	}param_;

	std::string reticleTexturePath_ = "Textures/reticle.png"; //< 照準テクスチャパス
};
