#pragma once
#include "Engine/Foundation/Math/Vector3.h"

/*-----------------------------------------------------------------------------------------
 * HomingTarget
 * - ホーミング対象クラス
 * - ホーミング弾が追尾するために中心座標などを返す
 *---------------------------------------------------------------------------------------*/
class HomingTarget {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**\brief コンストラクタ / デストラクタ*/
	HomingTarget() = default;
	virtual ~HomingTarget() = default;
	/**
	 * \brief 中心座標を取得
	 * \return 中心座標
	 */
	virtual const CalyxMath::Vector3 GetCenterPos() const { return centerPos_; }

private:
	CalyxMath::Vector3 centerPos_;
};
