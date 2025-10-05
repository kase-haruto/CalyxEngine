#pragma once

//* engine
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Objects/3D/Geometory/Shape.h>

#include <variant>
#include <string>

//===================================================================*/
//							  ColliderType
//===================================================================*/
enum class ColliderType{
	Type_None				= 0,        // ビットが立っていない状態
	Type_Player				= 1 << 1,
	Type_PlayerAttack		= 2 << 1,
	Type_Enemy				= 3 << 1,
	Type_EnemySpawner		= 4 << 1,
	Type_EnemyAttack		= 5 << 1,
};

// ビット演算のオーバーロード
inline ColliderType operator|(ColliderType lhs, ColliderType rhs){
	using T = std::underlying_type_t<ColliderType>;
	return static_cast< ColliderType >(static_cast< T >(lhs) | static_cast< T >(rhs));
}

inline ColliderType& operator|=(ColliderType& lhs, ColliderType rhs){
	lhs = lhs | rhs;
	return lhs;
}

// ビットAND演算のオーバーロード
inline ColliderType operator&(ColliderType lhs, ColliderType rhs){
	using T = std::underlying_type_t<ColliderType>;
	return static_cast< ColliderType >(static_cast< T >(lhs) & static_cast< T >(rhs));
}

// ビットAND代入演算のオーバーロード
inline ColliderType& operator&=(ColliderType& lhs, ColliderType rhs){
	lhs = lhs & rhs;
	return lhs;
}

class BaseGameObject; // 前方宣言

class Collider{
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	Collider() = default;
	Collider(bool isEnuble);
	virtual ~Collider();
	virtual void Update(const Vector3& position,const Quaternion& rotate) = 0;
	virtual void Draw() = 0;
	virtual void ShowGui();
	void ShowGui(struct ColliderConfig& config);


	virtual void OnCollisionEnter([[maybe_unused]] Collider* other){};
	virtual void OnCollisionStay([[maybe_unused]] Collider* other){};
	virtual void OnCollisionExit([[maybe_unused]] Collider* other){};

	void NotifyCollisionEnter(Collider* other);
	void NotifyCollisionStay(Collider* other);
	void NotifyCollisionExit(Collider* other);
	void ApplyConfig(const struct ColliderConfig& config);
	ColliderConfig ExtractConfig() const;

protected:
	//===================================================================*/
	//                   protected methods
	//===================================================================*/
	std::variant<Sphere, OBB> collisionShape_;
	std::string name_;

	ColliderType type_;							//< 自身のタイプ
	ColliderType targetType_;					//< 衝突相手のタイプ
	Vector4 color_ = {1.0, 0.0, 0.0, 1.0};		//< 描画色
	Vector3 offset_{ 0.0f, 0.0f, 0.0f };

	bool isCollisionEnabled_ = false;			//< 衝突判定を行うかどうか
	bool isDraw_ = true;						//< 描画を行うかどうか
	bool isTrigger_ = false;					//< 押し戻しなどを行うかどうか
public:
	//===================================================================*/
	//                   getter/setter
	//===================================================================*/

	void SetOwner(BaseGameObject* owner) { owner_ = owner; }
	BaseGameObject* GetOwner() const { return owner_; }

	virtual const Vector3& GetCenter()const = 0;
	virtual const std::variant<Sphere, OBB>& GetCollisionShape() = 0;

	const std::string& GetName()const{ return name_; }
	void SetName(const std::string& name){ name_ = name; }

	ColliderType GetType() const{ return type_; }
	ColliderType GetTargetType() const{ return targetType_; }
	void SetType(ColliderType type) { type_ = type; }
	void SetTargetType(ColliderType targetType) { targetType_ = targetType; }
	void SetColor(const Vector4& color){ color_ = color; }

	bool IsCollisionEnubled()const{ return isCollisionEnabled_; }
	void SetCollisionEnabled(bool isCollisionEnuble);

	void SetIsDrawCollider(bool isDraw) { isDraw_ = isDraw; }

	void SetOffset(const Vector3& off) { offset_ = off; }
	const Vector3& GetOffset() const { return offset_; }
	void AddOffset(const Vector3& d) { offset_ += d; }
private:
	BaseGameObject* owner_ = nullptr;
};


