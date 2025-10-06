#pragma once

// engine
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/Objects/Transform/Transform.h>

// math
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Quaternion.h>

// c++
#include <algorithm>
#include <cmath>

class SplineFollower {
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	enum class LookMode {
		None,			//< 向きを更新しない
		AlongPath,		//< スプラインの接続方向に向く
		TowardsTarget	//< targetTransformの位置を向く
	};

	void BindPath(const SplineData* path,
				  float startT = 0.0f, bool loop = false,
				  int arcSamplesPerSeg = 32);

	void Update(float dt);

	//--------- accessor -------------------------------------------------//
	Vector3     GetPosition()  const { return curPos_; }
	Quaternion  GetRotation()  const { return curRot_; }
	float       GetT()         const { return t_; }
	bool        IsFinished()   const { return finished_; }
	const SplineData* GetPath()const { return path_; }

	void SetWorldSpeed(float mPerSec)	{ worldSpeed_ = mPerSec; }
	void SetPathSpeed(float uPerSec)	{ pathSpeed_ = uPerSec; }
	void SetLookMode(LookMode m)		{ lookMode_ = m; }
	void SetYOffset(float y)			{ yOffset_ = y; }
	void SetLoop(bool loop)				{ loop_ = loop; }
	void SetTargetTransform(const WorldTransform* tf) { targetTransform_ = tf; }
	void SetArcSamplesPerSeg(int n);

private:
	//===================================================================*/
	//				private methods
	//===================================================================*/
	 // 入力
	const SplineData* path_ = nullptr;
	const WorldTransform* targetTransform_ = nullptr;

	// 状態
	float t_ = 0.0f;     // 0..1
	bool loop_ = true;
	bool finished_ = false;

	// 設定
	int arcSamplesPerSeg_ = 32;	// 分割密度
	float worldSpeed_ = 10.0f;	// m/s
	float pathSpeed_ = 0.1f;	// t/s（フォールバック）
	float yOffset_ = 0.0f;
	LookMode lookMode_ = LookMode::AlongPath;

	// 出力キャッシュ
	Quaternion curRot_ = Quaternion();		// 単位クォータニオン想定
	Vector3 curPos_ = Vector3(0, 0, 0);
	Vector3 lastPos_ = Vector3(0, 0, 0);
};

