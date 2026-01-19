#pragma once
#include "Engine/Foundation/Math/MathUtil.h"
#include <Engine/Foundation/Serialization/SerializableObject.h>

/*-------------------------------------------------------------
 *	ReticleMover
 *	- 照準移動クラス
 *	- プレイヤーのロックオン照準の移動を管理する
 *-----------------------------------------------------------*/
class ReticleMover {
public:
	//=============================================================*/
	// public method
	//=============================================================*/
	ReticleMover();
	~ReticleMover();

	/**
	 * \brief 更新処理
	 */
	void Update(float speed,float dt);
	/**
	 * \brief レティクルを移動させる
	 */
	const CalyxMath::Vector2& MoveOffset() const;

private:
	//=============================================================*/
	// private method
	//=============================================================*/
	CalyxMath::Vector2 moveOffset_ = CalyxMath::Vector2::Zero(); //< 移動オフセット
};
