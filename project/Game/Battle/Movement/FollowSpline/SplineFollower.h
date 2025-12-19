#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */

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
	/* ========================================================================
	/*  public methods
	/* ===================================================================== */

	//---------------------------------------------------------------
	//		向きモード
	//---------------------------------------------------------------
	enum class LookMode {
		None,			//< 向きを更新しない
		AlongPath,		//< スプラインの接線方向に向く
		TowardsTarget	//< targetTransform の位置を向く（ワールド空間）
	};

	//---------------------------------------------------------------
	//		経路バインド
	//---------------------------------------------------------------
	void BindPath(const SplineData* path,
				  float startT = 0.0f, bool loop = false,
				  int arcSamplesPerSeg = 32);

	//---------------------------------------------------------------
	//		更新
	//---------------------------------------------------------------
	void Update(float dt);

	//---------------------------------------------------------------
	//		アクセサ
	//---------------------------------------------------------------
	CxMath::Vector3     GetPosition()    const { return curPos_; }			//< 現在のワールド位置
	CxMath::Quaternion  GetRotation()    const { return curRot_; }			//< 現在のワールド回転
	float       GetT()           const { return t_; }				//< 現在の補間パラメータ
	bool        IsFinished()     const { return finished_; }			//< 終端に達したか
	const SplineData* GetPath()  const { return path_; }				//< 経路参照
	CxMath::Vector3     GetWorldPosition() const { return curPos_; }			//< エイリアス

	//---------------------------------------------------------------
	//		パラメータ設定
	//---------------------------------------------------------------
	void SetWorldSpeed(float mPerSec) { worldSpeed_ = mPerSec; }	// 距離[m]/s
	void SetPathSpeed(float uPerSec) { pathSpeed_ = uPerSec; }	// t/s（フォールバック）
	void SetLookMode(LookMode m) { lookMode_ = m; }
	void SetYOffset(float y) { yOffset_ = y; }
	void SetLoop(bool loop) { loop_ = loop; }
	void SetTargetTransform(const WorldTransform* tf) { targetTransform_ = tf; }

	//---------------------------------------------------------------
	//		アンカー設定（スポーナー基準対応）
	//---------------------------------------------------------------
	void SetAnchor(const WorldTransform* anchor, bool inheritPos = true, bool inheritRot = true);

	void SetArcSamplesPerSeg(int n);

private:
	/* ========================================================================
	/*  private fields
	/* ===================================================================== */

	const SplineData* path_ = nullptr;		//< 経路データ
	const WorldTransform* targetTransform_ = nullptr;		//< TowardsTarget 用
	const WorldTransform* anchor_ = nullptr;		//< スポーナー等の基準

	float t_ = 0.0f;		//< 0..1
	bool  loop_ = true;
	bool  finished_ = false;
	bool  inheritPos_ = true;	//< アンカーの位置を継承
	bool  inheritRot_ = true;	//< アンカーの回転を継承

	int    arcSamplesPerSeg_ = 32;	//< 分割密度（1セグメントあたり）
	float  worldSpeed_ = 10.0f;		//< m/s
	float  pathSpeed_ = 0.1f;		//< t/s（フォールバック）
	float  yOffset_ = 0.0f;
	LookMode lookMode_ = LookMode::AlongPath;

	CxMath::Quaternion curRot_ = CxMath::Quaternion();		//< 現在の回転
	CxMath::Vector3    curPos_ = CxMath::Vector3(0, 0, 0);	//< 現在の位置
	CxMath::Vector3    lastPos_ = CxMath::Vector3(0, 0, 0);	//< 前フレーム位置
};
