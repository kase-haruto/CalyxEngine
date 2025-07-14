#pragma once
#include "../Model/BaseModel.h"
#include "AnimationStruct.h"
#include <externals/imgui/imgui.h>
class AnimationModel
	: public BaseModel{
public:
	//===================================================================*/
	//                   public func
	//===================================================================*/
	AnimationModel() = default;
	AnimationModel(const std::string& fileName);
	~AnimationModel() override = default;

	void Initialize() override;
	void Update() override;
	void OnModelLoaded() override;
	void Draw(const WorldTransform& transform)override;
	void Map() override;
	void ShowImGuiInterface() override;
	void SkeletonUpdate();
	void SkinClusterUpdate();
	void DrawSkeleton();

	std::string GetCurrentAnimationName() const;

	// アニメーションを追加
	void AddAnimation(const std::string& animName, const std::string& fileName);
	// アニメーションを再生
	void PlayAnimation(const std::string& animName, float blendDuration);

	//============= 
	// Transform関連
	//=============
	// アニメーション速度の設定と取得
	void SetAnimationSpeed(float speed){ animationSpeed_ = speed; }
	float GetAnimationSpeed() const{ return animationSpeed_; }

	std::vector<std::string> GetAnimationNodeNames() const;
	std::optional<Matrix4x4> GetJointMatrix(const std::string& name) const;
private:
	//===================================================================*/
	//                   private func
	//===================================================================*/
	//============
	// バッファ生成/マップの実装
	//============
	void CreateMaterialBuffer() override;
	void MaterialBufferMap() override;

	// アニメーションを再生する
	void PlayAnimation();

	void BuildFastChannels(Animation& anim);
	void ApplyAnimationToSkeleton();

	Quaternion CalculateValue(const AnimationCurve<Quaternion>& curve, float time);
	Vector3 CalculateValue(const AnimationCurve<Vector3>& curve, float time);

	void SkinningStep();

private:
	//===================================================================*/
	//                    private variables
	//===================================================================*/
	float animationTime_ = 0.0f;		//< アニメーションの経過時間

	Animation animationData_;			//< アニメーションデータ
	int  selectedJoint_ = -1;
	ImVec4 jointHighlightCol_ = { 1.0f, 0.2f, 0.2f, 1.0f };
	SkinCluster skinCluster_;			//< スキンクラスター
	D3D12_VERTEX_BUFFER_VIEW vbvs_[2];	//< スキンクラスター用のバッファビュー
public:
	float animationSpeed_ = 1.0f;		//< アニメーションの再生速度
	bool isDrawSkeleton_ = false;		//< スケルトンを描画するかどうか

private:
	std::unordered_map<std::string, AnimationState> animationStates_;
	AnimationState* currentAnimation_ = nullptr;
	AnimationState* nextAnimation_ = nullptr;
	float blendTime_ = 0.0f;
	float blendDuration_ = 0.2f; // ブレンド時間（秒）
};
