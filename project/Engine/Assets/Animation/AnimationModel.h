#pragma once
#include "../Model/BaseModel.h"
#include "AnimationStruct.h"
#include <externals/imgui/imgui.h>

/* ========================================================================
/*		動的モデル
/* ===================================================================== */
class AnimationModel
	: public BaseModel {
public:
	//===================================================================*/
	//					public method
	//===================================================================*/
	AnimationModel() = default;
	AnimationModel(const std::string& fileName);
	~AnimationModel() override = default;

	void Initialize() override;
	void Update(float dt) override;
	void Draw(const WorldTransform& transform) override;
	void ShowImGuiInterface() override;

	// モデル読み込み時処理
	void OnModelLoaded() override;

	//--------- skeleton -----------------------------------------------------
	void SkeletonUpdate();
	void SkinClusterUpdate();
	void DrawSkeleton();

	// アニメーションを追加
	void AddAnimation(const std::string& animName, const std::string& fileName);
	// アニメーションを再生
	void PlayAnimation(const std::string& animName, float blendDuration);

	//--------- accessor ------------------------------------------------------
	// getter
	std::string				 GetCurrentAnimationName() const;
	float					 GetAnimationSpeed() const { return animationSpeed_; }
	std::vector<std::string> GetAnimationNodeNames() const;
	std::optional<Matrix4x4> GetJointMatrix(const std::string& name) const;

	// setter
	void SetAnimationSpeed(float speed) { animationSpeed_ = speed; }

private:
	//===================================================================*/
	//					private method
	//===================================================================*/
	void CreateMaterialBuffer() override;
	void MaterialBufferMap() override;
	void Map() override;

	/// <summary>
	/// アニメーション再生
	/// </summary>
	void PlayAnimation();

	/// <summary>
	/// アニメーションをバインド
	/// </summary>
	/// <param name="anim"></param>
	void BuildFastChannels(Animation& anim);

	/// <summary>
	/// スケルトンのアニメーションを適用
	/// </summary>
	void ApplyAnimationToSkeleton();

	/// <summary>
	/// アニメーションCurveを適用
	/// </summary>
	/// <param name="curve"></param>
	/// <param name="time"></param>
	/// <returns></returns>
	Quaternion CalculateValue(const AnimationCurve<Quaternion>& curve, float time);
	Vector3	   CalculateValue(const AnimationCurve<Vector3>& curve, float time);

	/// <summary>
	/// スケルトン計算
	/// </summary>
	void SkinningStep();

private:
	//===================================================================*/
	//                    private variables
	//===================================================================*/
	float animationTime_ = 0.0f; //< アニメーションの経過時間

	Animation				 animationData_; //< アニメーションデータ
	int						 selectedJoint_		= -1;
	ImVec4					 jointHighlightCol_ = {1.0f, 0.2f, 0.2f, 1.0f};
	SkinCluster				 skinCluster_; //< スキンクラスター
	D3D12_VERTEX_BUFFER_VIEW vbvs_[2];	   //< スキンクラスター用のバッファビュー
public:
	float animationSpeed_ = 1.0f;  //< アニメーションの再生速度
	bool  isDrawSkeleton_ = false; //< スケルトンを描画するかどうか

private:
	std::unordered_map<std::string, AnimationState> animationStates_;
	AnimationState*									currentAnimation_ = nullptr;
	AnimationState*									nextAnimation_	  = nullptr;
	float											blendTime_		  = 0.0f;
	float											blendDuration_	  = 0.2f; // ブレンド時間（秒）
};
