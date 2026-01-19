#pragma once
#include "../Model/BaseModel.h"
#include "AnimationStruct.h"
#include <externals/imgui/imgui.h>

namespace CalyxAssets {

	/*-----------------------------------------------------------------------------------------
	 * AnimationModel
	 * - アニメーションモデルクラス
	 * - スケルタルアニメーション付きモデルの再生・スキニング処理を担当
	 *---------------------------------------------------------------------------------------*/
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
		void BindVertexIndexBuffers(ID3D12GraphicsCommandList* cmdList)const override;
		void ShowImGuiInterface() override;

		// モデル読み込み時処理
		void OnModelLoaded() override;

		// アニメーションの登録 (ゲーム側は enum を int16_t にキャストして渡す想定)
		void RegisterAnimation(int16_t animID,const std::string& animName,const std::optional<std::string>& fileName = std::nullopt);
		// アニメーション再生 (ID ベース)
		void Play(int16_t id,float blend = 0.2f);
		// ワンショット再生（再生終了後に returnAnim へ戻す）
		void PlayOneShot(int16_t id,int16_t returnAnim,float blend = 0.1f);
		// ループ設定
		void SetLoop(int16_t id,bool isLoop);
		// アニメーション終了判定
		bool IsAnimationFinished() const;

		/**
		 * \brief スキニング用のジョイント行列 SRV をコマンドにセット
		 */
		void SetCommandPalletSrv(UINT rootParameterIndex,ID3D12GraphicsCommandList* cmdList) const;

		//--------- skeleton -----------------------------------------------------
		void SkeletonUpdate();
		void SkinClusterUpdate();
		void DrawSkeleton();

		// アニメーションを追加（名前ベース・従来 API）
		void AddAnimation(const std::string& animName,const std::string& fileName);
		// アニメーションを再生（名前ベース・従来 API）
		void PlayAnimation(const std::string& animName,float blendDuration);

		//--------- accessor ------------------------------------------------------//
		std::string                         GetCurrentAnimationName() const;
		float                               GetAnimationSpeed() const { return animationSpeed_; }
		std::vector<std::string>            GetAnimationNodeNames() const;
		std::optional<CalyxMath::Matrix4x4> GetJointMatrix(const std::string& name) const;
		D3D12_GPU_DESCRIPTOR_HANDLE         GetJointMatrixSrv() const;
		void                                SetAnimationSpeed(float speed) { animationSpeed_ = speed; }

	private:
		//===================================================================*/
		//					private method
		//===================================================================*/
		void CreateMaterialBuffer() override;
		void MaterialBufferMap() override;
		void Map() override;

		/// アニメーション再生（毎フレーム更新側）
		void PlayAnimation();

		/// アニメーションをバインド
		void BuildFastChannels(Animation& anim);



		/// アニメーションCurveを適用
		CalyxMath::Quaternion CalculateValue(const AnimationCurve<CalyxMath::Quaternion>& curve,float time, size_t& hint);
		CalyxMath::Vector3    CalculateValue(const AnimationCurve<CalyxMath::Vector3>& curve,float time, size_t& hint);

		/// スケルトン計算
		void SkinningStep();

	private:
		//===================================================================*/
		//                    private variables
		//===================================================================*/
		float animationTime_ = 0.0f; //< アニメーションの経過時間

		Animation                animationData_;
		int                      selectedJoint_     = -1;
		ImVec4                   jointHighlightCol_ = {1.0f,0.2f,0.2f,1.0f};
		SkinCluster              skinCluster_;
		mutable D3D12_VERTEX_BUFFER_VIEW vbvs_[2];

	public:
		float animationSpeed_ = 1.0f;  //< アニメーションの再生速度
		bool  isDrawSkeleton_ = false; //< スケルトンを描画するかどうか

	private:
		std::unordered_map<std::string,AnimationState> animationStates_;
		AnimationState*                                currentAnimation_ = nullptr;
		AnimationState*                                nextAnimation_    = nullptr;
		float                                          blendTime_        = 0.0f;
		float                                          blendDuration_    = 0.2f; // ブレンド時間（秒）

	private:
		// ID → アニメーション名
		std::unordered_map<int16_t,std::string> animIdTable_;
		bool                                    isOneShot_     = false;
		int16_t                                 oneShotReturn_ = 0;
	};

}