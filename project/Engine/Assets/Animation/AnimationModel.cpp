#include "AnimationModel.h"

#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <engine/graphics/Material.h>

#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Lighting/LightData.h>
#include <Engine/Renderer/Mesh/VertexData.h>

#ifdef _DEBUG
#include <externals/imgui/imgui.h>
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#endif

#include <Engine/Foundation/Utility/Func/MyFunc.h>

//-----------------------------------------------------------------------------
// コンストラクタ
//-----------------------------------------------------------------------------
AnimationModel::AnimationModel(const std::string& fileName){
	fileName_ = fileName;

	// 初期化
	Initialize();

	try{
		animationData_ = LoadAnimationFile("Resources/models", fileName_);

		std::string baseName = fileName;
		size_t pos = baseName.find_last_of('.');
		if (pos != std::string::npos){
			baseName = baseName.substr(0, pos);
		}

		//最初に読み込んだアニメーションファイルをアニメーションリストに追加
		AnimationState initialState;
		initialState.name = baseName;
		initialState.animation = animationData_;
		initialState.currentTime = 0.0f;
		initialState.speed = 1.0f;
		initialState.weight = 1.0f; // 最初なので 1.0
		initialState.loop = true;

		animationStates_[baseName] = initialState;
		currentAnimation_ = &animationStates_[baseName];

	} catch (...){
		// 失敗したら duration=0 にする
		modelData_->animation.duration = 0.0f;
	}
}


//-----------------------------------------------------------------------------
// 初期化
//-----------------------------------------------------------------------------
void AnimationModel::Initialize(){
	// バッファ生成
	CreateMaterialBuffer();
	Map();
}

//-----------------------------------------------------------------------------
// アニメーションの再生
//-----------------------------------------------------------------------------
void AnimationModel::PlayAnimation(){
	if (!currentAnimation_) return;

	float dt = ClockManager::GetInstance()->GetDeltaTime();

	// 現在アニメーション時間を進める
	currentAnimation_->currentTime += dt * currentAnimation_->speed;
	if (currentAnimation_->loop){
		currentAnimation_->currentTime = std::fmod(currentAnimation_->currentTime, currentAnimation_->animation.duration);
	}

	// ブレンド先がある場合
	if (nextAnimation_){
		nextAnimation_->currentTime += dt * nextAnimation_->speed;
		if (nextAnimation_->loop){
			nextAnimation_->currentTime = std::fmod(nextAnimation_->currentTime, nextAnimation_->animation.duration);
		}

		blendTime_ += dt;
		float blendFactor = std::clamp(blendTime_ / blendDuration_, 0.0f, 1.0f);

		currentAnimation_->weight = 1.0f - blendFactor;
		nextAnimation_->weight = blendFactor;

		if (blendFactor >= 1.0f){
			currentAnimation_ = nextAnimation_;
			nextAnimation_ = nullptr;
			currentAnimation_->weight = 1.0f;
		}
	} else{
		currentAnimation_->weight = 1.0f;
	}

	// 適用
	ApplyAnimationToSkeleton();
}

void AnimationModel::ApplyAnimationToSkeleton(){
	for (Joint& joint : modelData_->skeleton.joints){
		auto jointName = joint.name;

		QuaternionTransform blended {};
		bool hasAny = false;

		auto accumulate = [&] (AnimationState* state){
			if (!state || state->weight <= 0.0f) return;

			auto it = state->animation.nodeAnimations.find(jointName);
			if (it == state->animation.nodeAnimations.end()) return;

			const NodeAnimation& nodeAnim = it->second;

			if (!nodeAnim.translate.keyframes.empty()){
				Vector3 v = CalculateValue(nodeAnim.translate, state->currentTime);
				blended.translate += v * state->weight;
				hasAny = true;
			}

			if (!nodeAnim.rotate.keyframes.empty()){
				Quaternion q = CalculateValue(nodeAnim.rotate, state->currentTime);
				if (hasAny){
					blended.rotate = Quaternion::Slerp(blended.rotate, q, state->weight);
				} else{
					blended.rotate = q;
				}
				hasAny = true;
			}

			if (!nodeAnim.scale.keyframes.empty()){
				Vector3 s = CalculateValue(nodeAnim.scale, state->currentTime);
				blended.scale += s * state->weight;
				hasAny = true;
			}
			};

		accumulate(currentAnimation_);
		accumulate(nextAnimation_);

		if (hasAny){
			joint.transform = blended;
		}
	}
}

void AnimationModel::AddAnimation(const std::string& animName, const std::string& fileName){
	AnimationState state;
	state.name = animName;
	state.animation = LoadAnimationFile("Resources/models", fileName);
	state.currentTime = 0.0f;
	state.speed = 1.0f;
	state.weight = 0.0f;
	state.loop = true;

	animationStates_[animName] = state;

	if (!currentAnimation_){
		currentAnimation_ = &animationStates_[animName];
		currentAnimation_->weight = 1.0f;
	}
}

void AnimationModel::PlayAnimation(const std::string& animName, float blendDuration){
	auto it = animationStates_.find(animName);
	if (it == animationStates_.end()) return;

	nextAnimation_ = &it->second;
	blendTime_ = 0.0f;
	blendDuration_ = blendDuration;
}


Quaternion AnimationModel::CalculateValue(const AnimationCurve<Quaternion>& curve, float time){
	if (curve.keyframes.empty()){
		return Quaternion(0, 0, 0, 1); // identity
	}
	if (time <= curve.keyframes.front().time){
		return curve.keyframes.front().value;
	}
	if (time >= curve.keyframes.back().time){
		return curve.keyframes.back().value;
	}
	for (int i = 0; i < ( int ) curve.keyframes.size() - 1; ++i){
		float t0 = curve.keyframes[i].time;
		float t1 = curve.keyframes[i + 1].time;
		if (time >= t0 && time <= t1){
			float localT = (time - t0) / (t1 - t0);
			const Quaternion& q0 = curve.keyframes[i].value;
			const Quaternion& q1 = curve.keyframes[i + 1].value;
			return Quaternion::Slerp(q0, q1, localT); // 球面線形補間
		}
	}
	return curve.keyframes.back().value;
}

Vector3 AnimationModel::CalculateValue(const AnimationCurve<Vector3>& curve, float time){
	if (curve.keyframes.empty()){
		return Vector3(0, 0, 0);
	}
	if (time <= curve.keyframes.front().time){
		return curve.keyframes.front().value;
	}
	if (time >= curve.keyframes.back().time){
		return curve.keyframes.back().value;
	}
	for (int i = 0; i < ( int ) curve.keyframes.size() - 1; ++i){
		float t0 = curve.keyframes[i].time;
		float t1 = curve.keyframes[i + 1].time;
		if (time >= t0 && time <= t1){
			float localT = (time - t0) / (t1 - t0);
			const Vector3& v0 = curve.keyframes[i].value;
			const Vector3& v1 = curve.keyframes[i + 1].value;
			return Vector3::Lerp(v0, v1, localT); // 線形補間
		}
	}
	return curve.keyframes.back().value;
}

//-----------------------------------------------------------------------------
// 毎フレームの更新
//-----------------------------------------------------------------------------
void AnimationModel::SkeletonUpdate(){
	// すべてのjointを更新
	for (Joint& joint : modelData_->skeleton.joints){
		joint.localMatrix = MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);

		// 親の行列がある場合は、親の行列を掛け合わせる
		if (joint.parent){
			joint.skeletonSpaceMatrix = joint.localMatrix * modelData_->skeleton.joints[*joint.parent].skeletonSpaceMatrix;
		} else{
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}

}

void AnimationModel::SkinClusterUpdate(){
	for (size_t jointIndex = 0; jointIndex < modelData_->skeleton.joints.size(); ++jointIndex){
		assert(jointIndex < skinCluster_.inverseBindPoseMatrices.size());
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
			skinCluster_.inverseBindPoseMatrices[jointIndex] * modelData_->skeleton.joints[jointIndex].skeletonSpaceMatrix;
		skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
			Matrix4x4::Transpose(Matrix4x4::Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
	}
}

void AnimationModel::DrawSkeleton(){
	modelData_->skeleton.Draw();
}

void AnimationModel::Update(){
	if (modelData_){
		PlayAnimation();
		SkeletonUpdate();
		SkinClusterUpdate();
	}

	BaseModel::Update();
}

void AnimationModel::OnModelLoaded(){
	BaseModel::OnModelLoaded();
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

	modelData_->animation = animationData_;

	// スキンクラスターのリソースを確保
	skinCluster_ = CreateSkinCluster(device, modelData_->skeleton, *modelData_);

}

//-----------------------------------------------------------------------------
// 描画
//-----------------------------------------------------------------------------
void AnimationModel::Draw([[maybe_unused]]const WorldTransform& transform){
	// もしモデルデータが読み込まれていない場合は何もしない
	if (!modelData_){ return; }

	ID3D12GraphicsCommandList* cmdList = GraphicsGroup::GetInstance()->GetCommandList().Get();

	cmdList->SetGraphicsRootDescriptorTable(7, skinCluster_.paletteSrvHandle.second);

	// 頂点バッファ/インデックスバッファをセット
	vbvs_[0] = modelData_->vertexBuffer.GetVertexBufferView();	//vertexDataのvbv
	vbvs_[1] = skinCluster_.influenceBufferView;				//influenceDataのvbv
	modelData_->indexBuffer.SetCommand(cmdList);
	cmdList->IASetVertexBuffers(0, 2, vbvs_);
	BaseModel::Draw(transform);

	if (isDrawSkeleton_){
		modelData_->skeleton.Draw();
	}
}

//-----------------------------------------------------------------------------
// ImGui などUIの表示
//-----------------------------------------------------------------------------
void AnimationModel::ShowImGuiInterface(){
#ifdef _DEBUG

	GuiCmd::CheckBox("Draw Skeleton", isDrawSkeleton_);
	BaseModel::ShowImGuiInterface();

#endif
}

//-----------------------------------------------------------------------------
// バッファ生成/マッピング
//-----------------------------------------------------------------------------
void AnimationModel::Map(){
	MaterialBufferMap();
}

void AnimationModel::CreateMaterialBuffer(){
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	// materialData_ に初期値をセットする
	materialData_.color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData_.shininess = 20.0f;
	materialData_.enableLighting = HalfLambert;
	materialData_.uvTransform = Matrix4x4::MakeIdentity();

	// materialData_ の内容で GPU に転送
	materialBuffer_.Initialize(device);
}

void AnimationModel::MaterialBufferMap(){
	// materialData_ の内容で GPU に転送
	// マテリアルのデータを転送
	materialBuffer_.TransferData(materialData_);
}

//-----------------------------------------------------------------------------
// ノード名の取得例
//-----------------------------------------------------------------------------
std::vector<std::string> AnimationModel::GetAnimationNodeNames() const{
	std::vector<std::string> names;
	for (auto& pair : modelData_->animation.nodeAnimations){
		names.push_back(pair.first);
	}
	return names;
}
