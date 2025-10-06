#include "SplineFollower.h"

void SplineFollower::BindPath(const SplineData* path, float startT, bool loop, int arcSamplesPerSeg) {
	path_ = path;
	t_ = startT;
	loop_ = loop;
	arcSamplesPerSeg_ = (std::max)(4, arcSamplesPerSeg);
	if (path_) {
		const_cast<SplineData*>(path_)->BuildArcTable(arcSamplesPerSeg_);
	}
	finished_ = false;
}

void SplineFollower::Update(float dt) {
	lastPos_ = GetPosition(); // 一応保持

	if (!path_ || path_->SegmentCount() <= 0) {
		// 経路未設定：何もしない
		return;
	}

	// --- 前進（弧長ベース優先） ---
	if (path_->TotalLength() > 0.0f) {
		const float dist = (std::max)(0.0f, worldSpeed_) * dt;
		float newT = path_->AdvanceTBy(t_, dist);

		if (!path_->closed && !loop_) {
			// 非ループ：端で停止
			const float prevT = t_;
			t_ = std::clamp(newT, 0.0f, 1.0f);
			// 端に到達したら finished を立てる
			if ((prevT < 1.0f && t_ >= 1.0f) || (prevT > 0.0f && t_ <= 0.0f)) {
				finished_ = true;
			}
		} else {
			// ループ：0..1 を循環
			t_ = std::fmod(std::fmod(newT, 1.0f) + 1.0f, 1.0f);
		}
	} else {
		t_ += pathSpeed_ * dt;
		if (path_->closed || loop_) {
			t_ = std::fmod(std::fmod(t_, 1.0f) + 1.0f, 1.0f);
		} else {
			t_ = std::clamp(t_, 0.0f, 1.0f);
		}
	}

	// --- 位置 ---
	curPos_ = path_->Evaluate(t_);
	curPos_.y += yOffset_;

	// --- 向き ---
	switch (lookMode_) {
		case LookMode::None:
			{
				break;
			}
		case LookMode::AlongPath:
			{
				Vector3 fwd = path_->Tangent(t_);
				if (fwd.LengthSquared() < 1e-8f) fwd = Vector3(0, 0, 1);
				const float yaw = std::atan2(fwd.x, fwd.z);
				const float pitch = std::atan2(-fwd.y, std::sqrt(fwd.x * fwd.x + fwd.z * fwd.z));
				curRot_ = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
				break;
			}
		case LookMode::TowardsTarget:
			{
				Vector3 target = (targetTransform_) ? targetTransform_->GetWorldPosition() : curPos_;
				Vector3 d = target - curPos_;
				if (d.LengthSquared() > 1e-12f) d = d.Normalize();
				else d = Vector3(0, 0, 1);
				const float yaw = std::atan2(d.x, d.z);
				const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z));
				curRot_ = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
				break;
			}
	}
}

void SplineFollower::SetArcSamplesPerSeg(int n) {
	arcSamplesPerSeg_ = (std::max)(4, n);
	if (path_) const_cast<SplineData*>(path_)->BuildArcTable(arcSamplesPerSeg_);
}
