#pragma once

//* engine
#include "Collider.h"

/*-----------------------------------------------------------------------------------------
 * BoxCollider
 * - 箱型（OBB）衝突判定コライダークラス
 * - 直方体の形状を用いた衝突判定制御を担当
 *---------------------------------------------------------------------------------------*/
class BoxCollider : public Collider {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	BoxCollider() = default;

	/**
	 * \brief コンストラクタ
	 * \param isEnuble 有効フラグ
	 */
	BoxCollider(bool isEnuble);

	/**
	 * \brief デストラクタ
	 */
	~BoxCollider() override = default;

	/**
	 * \brief 更新処理
	 * \param position 座標
	 * \param rotate 回転
	 */
	void Update(const CalyxMath::Vector3& position, const CalyxMath::Quaternion& rotate) override;

	/**
	 * \brief 初期化
	 * \param size ボックスのサイズ（各軸の半幅）
	 */
	void Initialize(const CalyxMath::Vector3& size);

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
	virtual void OnCollisionStay([[maybe_unused]] Collider* other) override {};

	/**
	 * \brief 衝突終了時コールバック
	 * \param other 衝突相手
	 */
	virtual void OnCollisionExit([[maybe_unused]] Collider* other) override {};

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
	float GetColliderRadius() const override { return shape_.size.x * 0.5f; }

	/**
	 * \brief ボックスサイズを取得
	 * \return サイズ
	 */
	const CalyxMath::Vector3& GetSize() const { return shape_.size; }

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	OBB shape_; //< OBBデータ

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	std::string jsonPath = "gameobject"; //< 設定保存用パス

public:
	//===================================================================*/
	//                   getter/setter
	//===================================================================*/
	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	const CalyxMath::Vector3& GetCenter() const override;

	/**
	 * \brief ボックスサイズを設定
	 * \param size サイズ
	 */
	void SetSize(const CalyxMath::Vector3& size) { shape_.size = size; }

	/**
	 * \brief 衝突形状を取得
	 * \return 衝突形状
	 */
	const std::variant<Sphere, OBB>& GetCollisionShape() override;
};
