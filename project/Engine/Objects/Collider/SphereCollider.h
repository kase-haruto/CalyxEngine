#pragma once

//* engine
#include "Collider.h"

/*-----------------------------------------------------------------------------------------
 * SphereCollider
 * - 球体衝突判定コライダークラス
 * - 球の形状を用いた衝突判定制御を担当
 *---------------------------------------------------------------------------------------*/
class SphereCollider : public Collider {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	SphereCollider() = default;

	/**
	 * \brief コンストラクタ
	 * \param isEnuble 有効フラグ
	 */
	SphereCollider(bool isEnuble = true);

	/**
	 * \brief デストラクタ
	 */
	~SphereCollider() override = default;

	/**
	 * \brief 初期化
	 * \param radius 半径
	 */
	void Initialize(float radius);

	/**
	 * \brief 更新処理
	 * \param position 座標
	 * \param rotate 回転
	 */
	void Update(const CalyxEngine::Vector3& position, const CalyxEngine::Quaternion& rotate) override;

	/**
	 * \brief 描画処理
	 */
	void Draw() override;

	/**
	 * \brief ImGui表示
	 */
	void ShowGui() override;

	//* collision ==========================================*//
	/**
	 * \brief 衝突開始時コールバック
	 * \param other 衝突相手
	 */
	void OnCollisionEnter([[maybe_unused]] Collider* other) override {};

	/**
	 * \brief 衝突継続時コールバック
	 * \param other 衝突相手
	 */
	void OnCollisionStay([[maybe_unused]] Collider* other) override {};

	/**
	 * \brief 衝突終了時コールバック
	 * \param other 衝突相手
	 */
	void OnCollisionExit([[maybe_unused]] Collider* other) override {};

	//* config ==========================================*//
	/**
	 * \brief コンフィグを適用
	 * \param config コンフィグ
	 */
	void ApplyConfig(const struct ColliderConfig& config) override;

	/**
	 * \brief コンフィグを抽出
	 * \return コンフィグ
	 */
	ColliderConfig ExtractConfig() const override;

	/**
	 * \brief 衝突半径を取得
	 * \return 半径
	 */
	float GetColliderRadius() const override { return shape_.radius; }

	/**
	 * \brief 半径を設定
	 * \param radius 半径
	 */
	void SetRadius(float radius) { shape_.radius = radius; }

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	Sphere shape_; //< 球体データ

public:
	//===================================================================*/
	//                   getter/setter
	//===================================================================*/
	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	const CalyxEngine::Vector3& GetCenter() const override;

	/**
	 * \brief 衝突形状を取得
	 * \return 衝突形状
	 */
	const std::variant<Sphere, OBB>& GetCollisionShape() override;
};