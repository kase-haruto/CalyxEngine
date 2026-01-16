#pragma once
#include <Engine/Foundation/Math/Quaternion.h>

class PlayerDodgeSystem;
class WorldTransform;

/*-----------------------------------------------------------------------------------------
 * PlayerDodgeSpinMotion class
 * - 回避時の回転モーションを管理するクラス
 * - 回避開始/終了やパーフェクト回避に合わせて姿勢を更新する
 *---------------------------------------------------------------------------------------*/
class PlayerDodgeSpinMotion {
public:
	void Initialize(PlayerDodgeSystem* dodge, WorldTransform* wt);

	void OnDodgeStart();
	void OnDodgeEnd();
	void Update(float dt);
	void OnPerfect();

private:
	PlayerDodgeSystem* dodge_ = nullptr;
	WorldTransform*    wt_    = nullptr;

	CalyxMath::Quaternion baseRot_{0,0,0,1};
};
