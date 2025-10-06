#include "SplineFollower.h"

// ===============================================================
//  経路バインド（弧長LUT 構築）
// ===============================================================
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

// ===============================================================
//  更新（アンカー＝スポーナー基準 / 等速前進 / 向き更新）
// ===============================================================
void SplineFollower::Update(float dt) {
	lastPos_ = GetPosition(); // 一応保持（速度算出などに使いたい場合）

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
			// 端に到達したら finished_ を立てる
			if ((prevT < 1.0f && t_ >= 1.0f) || (prevT > 0.0f && t_ <= 0.0f)) {
				finished_ = true;
			}
		} else {
			// ループ：0..1 を循環
			t_ = std::fmod(std::fmod(newT, 1.0f) + 1.0f, 1.0f);
		}
	} else {
		// フォールバック：t/秒
		t_ += pathSpeed_ * dt;
		if (path_->closed || loop_) {
			t_ = std::fmod(std::fmod(t_, 1.0f) + 1.0f, 1.0f);
		} else {
			t_ = std::clamp(t_, 0.0f, 1.0f);
		}
	}

	// ===========================================================
	//  ローカル（スプライン空間）で位置・回転を算出
	// ===========================================================
	Vector3 localPos = path_->Evaluate(t_);
	localPos.y += yOffset_;

	// lookMode が TowardsTarget の場合、向きはワールド位置決定後に計算するので一旦 identity
	Quaternion localRot; // 単位クォータニオン
	switch (lookMode_) {
		case LookMode::None:
			{
				localRot = Quaternion(); // 単位
				break;
			}
		case LookMode::AlongPath:
			{
				Vector3 fwd = path_->Tangent(t_);
				if (fwd.LengthSquared() < 1e-8f) fwd = Vector3(0, 0, 1);
				const float yaw = std::atan2(fwd.x, fwd.z);
				const float pitch = std::atan2(-fwd.y, std::sqrt(fwd.x * fwd.x + fwd.z * fwd.z));
				localRot = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
				break;
			}
		case LookMode::TowardsTarget:
			{
				// 後でワールド位置が決まってから計算する
				localRot = Quaternion(); // ひとまず単位
				break;
			}
	}

	Vector3 worldPos;
	Quaternion worldRotPre = localRot;

	if (anchor_ && inheritPos_) {
		// アンカーのワールド行列でローカル点を変換（回転＋並進）
		worldPos = Vector3::Transform(localPos, anchor_->matrix.world);
	} else {
		worldPos = localPos;
	}

	if (anchor_ && inheritRot_) {
		// 回転は乗算で継承
		worldRotPre = anchor_->rotation * localRot;
	} else {
		worldRotPre = localRot;
	}

	if (lookMode_ == LookMode::TowardsTarget) {
		Vector3 target = (targetTransform_) ? targetTransform_->GetWorldPosition() : worldPos;
		Vector3 d = target - worldPos;
		if (d.LengthSquared() <= 1e-12f) d = Vector3(0, 0, 1);
		else d = d.Normalize();

		const float yaw = std::atan2(d.x, d.z);
		const float pitch = std::atan2(-d.y, std::sqrt(d.x * d.x + d.z * d.z));
		curRot_ = Quaternion::MakeRotateY(yaw) * Quaternion::MakeRotateX(pitch);
	} else {
		curRot_ = worldRotPre;
	}

	// 出力（ワールド）
	curPos_ = worldPos;
}

// ===============================================================
//  LUT 密度の動的変更
// ===============================================================
void SplineFollower::SetArcSamplesPerSeg(int n) {
	arcSamplesPerSeg_ = (std::max)(4, n);
	if (path_) const_cast<SplineData*>(path_)->BuildArcTable(arcSamplesPerSeg_);
}

// ===============================================================
//  アンカー（スポーナー）設定
// ===============================================================
void SplineFollower::SetAnchor(const WorldTransform* anchor, bool inheritPos, bool inheritRot) {
	anchor_ = anchor;
	inheritPos_ = inheritPos;
	inheritRot_ = inheritRot;
}
