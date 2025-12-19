#pragma once
/* ========================================================================
/* include
/* ===================================================================== */
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineData.h>
#include <Engine/Objects/3D/Geometory/Spline/SplineJson.h>

#include <string>
#include <vector>

class RailCamera
	: public BaseCamera {
public:
	RailCamera();
	RailCamera(const std::string& name);
	~RailCamera() = default;

	void Initialize();
	void Update(float dt) override;
	void ShowGui()override;
	void AlwaysUpdate(float dt)override;

	// 位置/姿勢
	CxMath::Vector3 GetPosition();
	const CxMath::Vector3& GetRotation() const { return worldTransform_.eulerRotation; }
	const WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// スプライン設定API
	void SetSpline(const SplineData& s);
	bool LoadSplineFromJson(const std::string& path);
	void ClearSpline();

	// パラメータ
	void SetSpeed(float s) { speed_ = s; }					// 単位: ユニット/秒（弧長）
	void SetLookAhead(float d) { lookAhead_ = d; }			// 先読み距離（向き計算）
	void SetBankScale(float rad) { tiltAngle_ = rad; }		// 最大ロール
	void SetBankLerp(float spd) { tiltLerpSpeed_ = spd; }
	void SetTilt(float angleRad, float lerp = 10.0f);
	void SetClosed(bool closed);
	void SetStopRatio(float r);

	float GetT() const;
	float GetProgress() const;

	std::string_view GetTypeName() const override { return "RailCamera"; }

private:
	
	// 進行方向とロール
	void UpdateOrientationFromPath(float dt);

private:
	// スプライン
	SplineData spline_;
	int arcSamplesPerSeg_ = 32;		// 1セグメント間で32分割
	float totalLength_ = 0.0f;

	// 状態
	float traveled_ = 0.0f;			// 走行弧長（0〜totalLength）
	float speed_ = 20.0f;			// 等速（弧長ベース）
	float lookAhead_ = 2.0f;		// 先読み距離（向き用）

	// ロール（左右傾き）
	float zTiltOffset_ = 0.0f;		// 現在の傾き
	float targetTilt_ = 0.0f;		// 目標の傾き（曲率由来）
	float tiltAngle_ = 0.3f;		// 最大傾き（ラジアン）
	float tiltLerpSpeed_ = 10.0f;	// 傾き補間速度

	// 
	float stopRatio_ = 0.9f;		//停止
};
