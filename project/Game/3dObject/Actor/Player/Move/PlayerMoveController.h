#pragma once
#include "Engine/Foundation/Math/Vector3.h"
#include "Engine/Objects/Transform/Transform.h"

class PlayerMoveController {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	PlayerMoveController();
	~PlayerMoveController();

	/**
	 * \brief 移動量を追加する
	 * \param delta
	 */
	void AddMove(const CxMath::Vector3& delta);
	/**
	 * \brief ワールド変換に移動を適用する
	 * \param wt
	 */
	void Apply(WorldTransform& wt);

private:
	//===================================================================*/
	//			private methods
	//===================================================================*/
	CxMath::Vector3 pendingMove_ = CxMath::Vector3::Zero();
};