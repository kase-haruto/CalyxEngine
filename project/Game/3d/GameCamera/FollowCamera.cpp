#include "FollowCamera.h"

/////////////////////////////////////////////////////////////////////////
//		ctor / dtor
/////////////////////////////////////////////////////////////////////////
FollowCamera::FollowCamera() {}
FollowCamera::FollowCamera(const std::string& name) :Camera3d(name){

}
FollowCamera::~FollowCamera() = default;

/////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////
void FollowCamera::Update(float dt) {
	if(followEnabled_ && getTargetPosW_) { UpdateFollow(dt); }

	Camera3d::Update(dt); // シェイク処理など
}

/////////////////////////////////////////////////////////////////////////
//		パラメータ調整
/////////////////////////////////////////////////////////////////////////
void FollowCamera::DerivativeGui() {}

/////////////////////////////////////////////////////////////////////////
//  アクセッサ
/////////////////////////////////////////////////////////////////////////
void FollowCamera::SetTargetProviders(std::function<Vector3()> getPosW,std::function<Vector3()> getFwdW) {
	getTargetPosW_ = std::move(getPosW);
	getTargetFwdW_ = std::move(getFwdW);
}

void FollowCamera::SetParams(float distance,float height,float lookAhead,float posHz,float posDamp,float rotResp) {
	followDistance_    = distance;
	followHeight_      = height;
	followLookAhead_   = lookAhead;
	followPosHz_       = posHz;
	followPosDamp_     = posDamp;
	followRotResponse_ = rotResp;
}

/////////////////////////////////////////////////////////////////////////
//		追尾更新
/////////////////////////////////////////////////////////////////////////
void FollowCamera::UpdateFollow(float dt) {
	// ターゲット取得（World）
	const Vector3 targetW = getTargetPosW_ ? getTargetPosW_() : GetWorldTransform().GetWorldPosition();
	Vector3       fwdW    = getTargetFwdW_ ? getTargetFwdW_() : Vector3(0,0,1);
	if(fwdW.LengthSquared() < 1e-6f) fwdW = Vector3(0,0,1);
	fwdW = fwdW.Normalize();

	// 注視点（lookAhead 先を見る）
	const Vector3 aimW = targetW + fwdW * followLookAhead_;

	// 初回スナップ（ポップ抑制）
	if(!followInitialized_) {
		// 先に向きを決める（今のpos→aimを見る）
		const Vector3 currentPosW = GetWorldTransform().GetWorldPosition();
		followRotW_               = Quaternion::LookAt(currentPosW,aimW,{0,1,0});
		// 向きの -Z 方向に距離、高さを足す
		const Vector3 back = Quaternion::RotateVector({0,0,-1},followRotW_);
		followPosW_        = targetW - back * followDistance_ + Vector3(0,followHeight_,0);
		followVelW_        = {};
		followInitialized_ = true;
	}

	// 回転：Slerpで「向きながら」
	const Quaternion desiredRot = Quaternion::LookAt(followPosW_,aimW,{0,1,0});
	const float      aRot       = std::clamp(dt * followRotResponse_,0.0f,1.0f);
	followRotW_                 = Quaternion::Slerp(followRotW_,desiredRot,aRot);

	// 位置：回転から -Z を算出し、理想位置をスプリングで「後追い補完」
	const Vector3 backNow = Quaternion::RotateVector({0,0,-1},followRotW_);
	const Vector3 goalW   = targetW - backNow * followDistance_ + Vector3(0,followHeight_,0);
	SpringStep(followPosHz_,followPosDamp_,dt,goalW,followPosW_,followVelW_);

	// 親がある場合は“親ローカル”へ変換して worldTransform_ にセット
	if(auto* parent = GetWorldTransform().parent) {
		parent->Update(); // 親のworldを最新に
		const Matrix4x4 parentW    = parent->matrix.world;
		const Matrix4x4 parentWInv = Matrix4x4::Inverse(parentW);

		// 位置：World→親ローカル
		const Vector3 localPos = Vector3::Transform(followPosW_,parentWInv);

		// 回転：World→親ローカル（行列で合成→Quat化）
		const Matrix4x4  rotW     = Quaternion::ToMatrix(followRotW_);
		const Matrix4x4  rotLocal = parentWInv * rotW; // 列/行の規約に合わせて乗算順は調整
		const Quaternion localRot = Quaternion::FromMatrix(rotLocal);

		worldTransform_.translation    = localPos;
		worldTransform_.rotation       = localRot;
		worldTransform_.rotationSource = RotationSource::Quaternion;
	} else {
		// 親なし：Worldをそのままローカルとして入れる
		worldTransform_.translation    = followPosW_;
		worldTransform_.rotation       = followRotW_;
		worldTransform_.rotationSource = RotationSource::Quaternion;
	}

	// シェイク基準位置（Camera3d/BaseCamera が使うなら整合のため更新）
	originalPosition_ = worldTransform_.translation;

}