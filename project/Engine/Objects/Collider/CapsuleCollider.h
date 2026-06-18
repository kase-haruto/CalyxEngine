#pragma once

//* engine
#include "Collider.h"

/*-----------------------------------------------------------------------------------------
 * CapsuleCollider
 * - カプセル形状の衝突判定コライダー
 * - ローカルY軸を中心軸とし、半球を含む全体高さと半径で形状を管理する
 *---------------------------------------------------------------------------------------*/
class CapsuleCollider : public Collider {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	CapsuleCollider() = default;

	/**
	 * \brief コンストラクタ
	 * \param isEnuble 有効フラグ
	 */
	CapsuleCollider(bool isEnuble);

	/**
	 * \brief デストラクタ
	 */
	~CapsuleCollider() override = default;

	/**
	 * \brief 初期化
	 * \param radius 半径
	 * \param height 半球を含めた全体高さ
	 */
	void Initialize(float radius, float height);

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
	 * \brief 衝突半径（概算）を取得
	 * \return 半径
	 */
	float GetColliderRadius() const override;

	/**
	 * \brief 半径を設定
	 * \param radius 半径
	 */
	void SetRadius(float radius) { shape_.radius = radius; }

	/**
	 * \brief 高さを設定
	 * \param height 半球を含めた全体高さ
	 */
	void SetHeight(float height) { shape_.height = height; }

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	Capsule shape_; //< カプセルデータ

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
	 * \brief 半径を取得
	 * \return 半径
	 */
	float GetRadius() const { return shape_.radius; }

	/**
	 * \brief 高さを取得
	 * \return 半球を含めた全体高さ
	 */
	float GetHeight() const { return shape_.height; }

	/**
	 * \brief 衝突形状を取得
	 * \return 衝突形状
	 */
	const CollisionShape& GetCollisionShape() override;
};
