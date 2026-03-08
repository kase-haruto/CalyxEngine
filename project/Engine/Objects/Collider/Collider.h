#pragma once

//* engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Geometory/Shape.h>

#include <string>
#include <variant>

//===================================================================*/
//							  ColliderType
//===================================================================*/
enum class ColliderType {
	Type_None		  = 0,
	Type_Player		  = 1 << 0, // 0b00000001
	Type_PlayerAttack = 1 << 1, // 0b00000010
	Type_Enemy		  = 1 << 2, // 0b00000100
	Type_EnemySpawner = 1 << 3, // 0b00001000
	Type_EnemyAttack  = 1 << 4, // 0b00010000
	Type_EventObject  = 1 << 5,
	Type_StageGimmick = 1 << 6,
};

// ビット演算のオーバーロード
inline ColliderType operator|(ColliderType lhs, ColliderType rhs) {
	using T = std::underlying_type_t<ColliderType>;
	return static_cast<ColliderType>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

inline ColliderType& operator|=(ColliderType& lhs, ColliderType rhs) {
	lhs = lhs | rhs;
	return lhs;
}

// ビットAND演算のオーバーロード
inline ColliderType operator&(ColliderType lhs, ColliderType rhs) {
	using T = std::underlying_type_t<ColliderType>;
	return static_cast<ColliderType>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

// ビットAND代入演算のオーバーロード
inline ColliderType& operator&=(ColliderType& lhs, ColliderType rhs) {
	lhs = lhs & rhs;
	return lhs;
}

class BaseGameObject; // 前方宣言

/*-----------------------------------------------------------------------------------------
 * Collider
 * - 衝突判定用コライダー基底クラス
 * - 各種衝突形状（球、OBB等）の管理、衝突イベント通知を担当
 *---------------------------------------------------------------------------------------*/
class Collider {
public:
	using CollisionCallback = std::function<void(Collider* other)>;

	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	Collider() = default;

	/**
	 * \brief コンストラクタ
	 * \param isEnuble 有効フラグ
	 */
	Collider(bool isEnuble);

	/**
	 * \brief デストラクタ
	 */
	virtual ~Collider();

	/**
	 * \brief 更新処理
	 * \param position 座標
	 * \param rotate 回転
	 */
	virtual void Update(const CalyxMath::Vector3& position, const CalyxMath::Quaternion& rotate) = 0;

	/**
	 * \brief 描画処理
	 */
	virtual void Draw() = 0;

	/**
	 * \brief ImGui表示
	 */
	virtual void ShowGui();

	/**
	 * \brief ImGui表示（コンフィグ同期）
	 * \param config コンフィグ
	 */
	void ShowGui(struct ColliderConfig& config);

	//* 衝突時処理 ==========================================*//
	/**
	 * \brief 衝突開始時コールバック
	 * \param other 衝突相手
	 */
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) {};

	/**
	 * \brief 衝突継続時コールバック
	 * \param other 衝突相手
	 */
	virtual void OnCollisionStay([[maybe_unused]] Collider* other) {};

	/**
	 * \brief 衝突終了時コールバック
	 * \param other 衝突相手
	 */
	virtual void OnCollisionExit([[maybe_unused]] Collider* other) {};

	//* 衝突通知 ==========================================*//
	/**
	 * \brief 衝突開始を通知
	 * \param other 衝突相手
	 */
	void NotifyCollisionEnter(Collider* other);

	/**
	 * \brief 衝突継続を通知
	 * \param other 衝突相手
	 */
	void NotifyCollisionStay(Collider* other);

	/**
	 * \brief 衝突終了を通知
	 * \param other 衝突相手
	 */
	void NotifyCollisionExit(Collider* other);

	/**
	 * \brief 衝突開始時コールバックを設定
	 * \param cb コールバック
	 */
	void SetOnEnter(CollisionCallback cb) { onEnter_ = std::move(cb); }

	/**
	 * \brief 衝突継続時コールバックを設定
	 * \param cb コールバック
	 */
	void SetOnStay(CollisionCallback cb) { onStay_ = std::move(cb); }

	/**
	 * \brief 衝突終了時コールバックを設定
	 * \param cb コールバック
	 */
	void SetOnExit(CollisionCallback cb) { onExit_ = std::move(cb); }

	//* config ==========================================*//
	/**
	 * \brief コンフィグを適用
	 * \param config コンフィグ
	 */
	virtual void ApplyConfig(const struct ColliderConfig& config);

	/**
	 * \brief コンフィグを抽出
	 * \return コンフィグ
	 */
	virtual ColliderConfig ExtractConfig() const;

	//* accessor ==========================================*//
	/**
	 * \brief 衝突半径を取得
	 * \return 半径
	 */
	virtual float GetColliderRadius() const = 0;

protected:
	//===================================================================*/
	//                    protected member variables
	//===================================================================*/
	std::variant<Sphere, OBB> collisionShape_; //< 衝突形状
	std::string				  name_;		   //< コライダー名

	ColliderType	   type_;							//< 自身のタイプ
	ColliderType	   targetType_;						//< 衝突相手のタイプ
	CalyxMath::Vector4 color_ = {1.0, 0.0, 0.0, 1.0};	//< 描画色
	CalyxMath::Vector3 offset_{0.0f, 0.0f, 0.0f};		//< オフセット座標
	CalyxMath::Vector3 rotateOffset_{0.0f, 0.0f, 0.0f}; //< 回転オフセット (Euler)

	bool isCollisionEnabled_ = false; //< 衝突判定を行うかどうか
	bool isDraw_			 = true;  //< 描画を行うかどうか
	bool isTrigger_			 = false; //< トリガーかどうか

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	CollisionCallback onEnter_; //< 衝突開始時コールバック
	CollisionCallback onStay_;	//< 衝突継続時コールバック
	CollisionCallback onExit_;	//< 衝突終了時コールバック

public:
	//===================================================================*/
	//                   getter/setter
	//===================================================================*/

	/**
	 * \brief 所有者を設定
	 * \param owner 所有者
	 */
	void SetOwner(BaseGameObject* owner) { owner_ = owner; }

	/**
	 * \brief 所有者を取得
	 * \return 所有者
	 */
	BaseGameObject* GetOwner() const { return owner_; }

	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	virtual const CalyxMath::Vector3& GetCenter() const = 0;

	/**
	 * \brief 衝突形状を取得
	 * \return 衝突形状
	 */
	virtual const std::variant<Sphere, OBB>& GetCollisionShape() = 0;

	/**
	 * \brief ワールド座標を取得
	 * \return ワールド座標
	 */
	CalyxMath::Vector3 GetWorldPos() const;

	/**
	 * \brief 名前を取得
	 * \return 名前
	 */
	const std::string& GetName() const { return name_; }

	/**
	 * \brief 名前を設定
	 * \param name 名前
	 */
	void SetName(const std::string& name) { name_ = name; }

	/**
	 * \brief タイプを取得
	 * \return タイプ
	 */
	ColliderType GetType() const { return type_; }

	/**
	 * \brief ターゲットタイプを取得
	 * \return ターゲットタイプ
	 */
	ColliderType GetTargetType() const { return targetType_; }

	/**
	 * \brief タイプを設定
	 * \param type タイプ
	 */
	void SetType(ColliderType type) { type_ = type; }

	/**
	 * \brief ターゲットタイプを設定
	 * \param targetType ターゲットタイプ
	 */
	void SetTargetType(ColliderType targetType) { targetType_ = targetType; }

	/**
	 * \brief 描画色を設定
	 * \param color 描画色
	 */
	void SetColor(const CalyxMath::Vector4& color) { color_ = color; }

	/**
	 * \brief 衝突判定が有効かを取得
	 * \return 有効か
	 */
	bool IsCollisionEnubled() const { return isCollisionEnabled_; }

	/**
	 * \brief 衝突判定の有効状態を設定
	 * \param isCollisionEnuble 有効にするか
	 */
	void SetCollisionEnabled(bool isCollisionEnuble);

	/**
	 * \brief 描画するかを設定
	 * \param isDraw 描画するか
	 */
	void SetIsDrawCollider(bool isDraw) { isDraw_ = isDraw; }

	/**
	 * \brief オフセットを設定
	 * \param off オフセット
	 */
	void SetOffset(const CalyxMath::Vector3& off) { offset_ = off; }

	/**
	 * \brief オフセットを取得
	 * \return オフセット
	 * /
	const CalyxMath::Vector3& GetOffset() const { return offset_; }

	/**
	 * \brief 回転オフセットを設定
	 * \param rot 回転オフセット
	 */
	void SetRotateOffset(const CalyxMath::Vector3& rot) { rotateOffset_ = rot; }

	/**
	 * \brief 回転オフセットを取得
	 * \return 回転オフセット
	 */
	const CalyxMath::Vector3& GetRotateOffset() const { return rotateOffset_; }

	/**
	 * \brief オフセットを加算
	 * \param d 加算量
	 */
	void AddOffset(const CalyxMath::Vector3& d) { offset_ += d; }

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	BaseGameObject* owner_ = nullptr; //< 所有オブジェクト
};