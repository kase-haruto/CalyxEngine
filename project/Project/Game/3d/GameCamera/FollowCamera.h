#pragma once
#include <Engine/Graphics/Camera/3d/Camera3d.h>

/**
 * 追従カメラ
 */
class FollowCamera final
	: public Camera3d {
public:
	//===================================================================*/
	//				public methods
	//===================================================================*/
	FollowCamera();
	FollowCamera(const std::string& name);
	~FollowCamera() override;

	void Update(float dt) override;
	void DerivativeGui() override;

	//--------- accessor -----------------------------------------------------
	void EnableFollow(bool on) { followEnabled_ = on; if(!on) followInitialized_ = false; }
	bool IsFollowEnabled() const { return followEnabled_; }
	void ResetFollow(){ followInitialized_ = false; followVelW_ = {}; }
	void SetTargetProviders(std::function<Vector3()> getPosW,
							std::function<Vector3()> getFwdW);
	void SetParams(float distance, float height, float lookAhead,
				   float posHz, float posDamp, float rotResp);

private:
	//===================================================================*/
	//				private methods
	//===================================================================*/
	void UpdateFollow(float dt);

	// 2次系スプリング
	static void SpringStep(float          hz,float        damp,float dt,
						   const Vector3& target,Vector3& x,Vector3& v) {
		const float omega = 2.0f * 3.14159265359f * (hz > 0.0f ? hz : 1e-3f);
		const float f2    = omega * omega;
		const float twoZ  = 2.0f * std::clamp(damp,0.0f,1.0f) * omega;
		v += ((target - x) * f2 - v * twoZ) * dt;
		x += v * dt;
	}

private:
	//===================================================================*/
	//				private methods
	//===================================================================*/

	// 有効フラグ・状態
	bool followEnabled_     = true;
	bool followInitialized_ = false;

	// ターゲット取得
	std::function<Vector3()> getTargetPosW_;
	std::function<Vector3()> getTargetFwdW_;

	// パラメータ（デフォルト）
	float followDistance_    = 8.0f;
	float followHeight_      = 3.0f;
	float followLookAhead_   = 1.0f;
	float followPosHz_       = 2.2f;
	float followPosDamp_     = 0.85f;
	float followRotResponse_ = 8.0f;

	// 状態
	Vector3    followPosW_ = Vector3::Zero();
	Vector3    followVelW_ = Vector3::Zero();
	Quaternion followRotW_ = Quaternion::MakeIdentity();
};