#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Game/3dObject/Actor/Bullet/EnemyBullet/BaseEnemyHomingBullet.h>
#include <string>

/*-----------------------------------------------------------------------------------------
 * EnemyHomingBullet
 * - 敵ホーミング弾クラス
 * - プレイヤーを追尾するホーミング弾の挙動を実装
 *---------------------------------------------------------------------------------------*/
class EnemyHomingBullet
	: public BaseEnemyHomingBullet {
public:
	//===================================================================*/
	//						public methods
	//===================================================================*/
	EnemyHomingBullet() = default;
	EnemyHomingBullet(const std::string& modelName, const std::string& name);
	~EnemyHomingBullet() override;
};
