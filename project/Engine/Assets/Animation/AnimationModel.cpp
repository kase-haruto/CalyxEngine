#include "AnimationModel.h"
#include <CalyxEngine/Project.h>
#include <Engine/Foundation/Debug/CxAssert.h>

#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <engine/graphics/Material.h>

#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Clock/ClockManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Lighting/LightData.h>
#include <Engine/Renderer/Mesh/VertexData.h>

#if defined(_DEBUG) || defined(DEVELOP)
#include <Engine/System/Command/EditorCommand/GuiCommand/ImGuiHelper/GuiCmd.h>
#include <externals/imgui/imgui.h>
#endif

#include "Engine/Foundation/Math/MathUtil.h"

#include <Engine/Foundation/Utility/Func/CxUtils.h>
#include <Engine/Foundation/Utility/Func/MyFunc.h>
#include <d3dx12.h>
#include <filesystem>

namespace CalyxEngine {
	/* =====================================================================
	   ctor – 最初に読み込んだファイルを初期アニメとして登録
	   ===================================================================*/
	AnimationModel::AnimationModel(const std::string& fileName) {
		fileName_ = fileName;
		CreateMaterialBuffer();
		Map();

		// メインアニメをロード
		animationData_ = LoadAnimationFile(Calyx::ResolveAssetPath("models").generic_string(), fileName_);

		// 初期ステートを登録
		std::string	   base = std::filesystem::path(fileName).stem().string();
		AnimationState st{base, animationData_};
		animationStates_.emplace(base, st);
		currentAnimation_ = &animationStates_.at(base);
	}

	AnimationModel::~AnimationModel() {
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, sourceVertexSrv_);
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, influenceSrv_);
		DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, skinCluster_.paletteSrvDescriptor);
	}

	/* =====================================================================
	   初期化 – マテリアルバッファなど
	   ===================================================================*/
	void AnimationModel::Initialize() {}

	void AnimationModel::Update(float dt) {
		if(modelData_) {
			PlayAnimation();
			SkinningStep();
		}

		BaseModel::Update(dt);
	}

	/* =====================================================================
	   毎フレーム呼び出し – 時間経過 & ブレンド
	   ===================================================================*/
	void AnimationModel::PlayAnimation() {
		if(!currentAnimation_) return;

		float dt = ClockManager::GetInstance()->GetDeltaTime();

		auto stepTime = [&](AnimationState* s) {
			if(!s) return;
			s->currentTime += dt * s->speed;
			if(s->loop && s->animation.duration > 0)
				s->currentTime = std::fmod(s->currentTime, s->animation.duration);
		};
		stepTime(currentAnimation_);
		stepTime(nextAnimation_);

		/* ブレンド係数更新 ------------------------------------------------*/
		if(nextAnimation_) {
			blendTime_ += dt;
			float f					  = std::clamp(blendTime_ / blendDuration_, 0.f, 1.f);
			currentAnimation_->weight = 1.f - f;
			nextAnimation_->weight	  = f;

			if(f >= 1.f) {
				currentAnimation_		  = nextAnimation_;
				nextAnimation_			  = nullptr;
				currentAnimation_->weight = 1.f;
			}
		} else {
			currentAnimation_->weight = 1.f;
		}
	}

	/* =====================================================================
	   Skeleton への適用 – restPose 差分 + 正規化
	   ===================================================================*/

	/* =====================================================================
	   Animation → JointIndex 直通テーブルを作る
	   ===================================================================*/
	void AnimationModel::BuildFastChannels(Animation& anim) {
		if(!modelData_) return;
		const Skeleton& sk = modelData_->skeleton;
		anim.fastChannels.assign(sk.joints.size(), nullptr);
		for(auto& [name, node] : anim.nodeAnimations) {
			auto it = sk.jointMap.find(name);
			if(it != sk.jointMap.end()) anim.fastChannels[it->second] = &node;
		}
	}

	/* =====================================================================
	   公開 API：外部から追加でアニメをロード
	   ===================================================================*/
	void AnimationModel::AddAnimation(const std::string& name, const std::string& file) {
		AnimationState st;
		st.name		 = name;
		st.animation = LoadAnimationFile(Calyx::ResolveAssetPath("models").generic_string(), file);
		if(modelData_) BuildFastChannels(st.animation);
		animationStates_.emplace(name, st);
	}

	/* =====================================================================
	   公開 API：アニメ再生リクエスト
	   ===================================================================*/
	void AnimationModel::PlayAnimation(const std::string& name, float dur) {
		if(currentAnimation_ && currentAnimation_->name == name) return;
		if(nextAnimation_ && nextAnimation_->name == name) return;

		auto it = animationStates_.find(name);
		if(it == animationStates_.end()) return;

		nextAnimation_				= &it->second;
		nextAnimation_->currentTime = 0.f;
		nextAnimation_->weight		= 0.f;
		blendTime_					= 0.f;
		blendDuration_				= dur;
	}

	/* =====================================================================
	   キーフレーム補間（upper_bound で O(log F)）
	   ===================================================================*/
	/* =====================================================================
	   キーフレーム補間（Hint付き線形探索 + upper_bound フォールバック）
	   ===================================================================*/
	template <typename T>
	static T LerpGeneric(const T& a, const T& b, float t) { return a + (b - a) * t; }

	// 特殊化が必要ならここで実装 (Slerpなどは内部で呼ぶ)

	CalyxEngine::Quaternion AnimationModel::CalculateValue(const AnimationCurve<CalyxEngine::Quaternion>& c, float t, size_t& hint) {
		if(c.keyframes.empty()) return CalyxEngine::Quaternion::MakeIdentity();
		if(t <= c.keyframes.front().time) {
			hint = 0;
			return c.keyframes.front().value;
		}
		if(t >= c.keyframes.back().time) {
			hint = c.keyframes.size() - 1;
			return c.keyframes.back().value;
		}

		// ヒント位置から探索スタート
		// 時間が戻った場合はリセット (ループ対応)
		if(hint >= c.keyframes.size() || c.keyframes[hint].time > t) {
			hint = 0;
		}

		// 線形探索 (フレームが進むのが前提なので、通常は数回でヒットする)
		while(hint + 1 < c.keyframes.size() && t >= c.keyframes[hint + 1].time) {
			hint++;
		}

		size_t i0 = hint;
		size_t i1 = i0 + 1;

		// 念のため範囲チェック
		if(i1 >= c.keyframes.size()) {
			return c.keyframes[i0].value;
		}

		float lT = (t - c.keyframes[i0].time) / (c.keyframes[i1].time - c.keyframes[i0].time);
		return CalyxEngine::Quaternion::Slerp(c.keyframes[i0].value, c.keyframes[i1].value, lT);
	}

	CalyxEngine::Vector3 AnimationModel::CalculateValue(const AnimationCurve<CalyxEngine::Vector3>& c, float t, size_t& hint) {
		if(c.keyframes.empty()) return {};
		if(t <= c.keyframes.front().time) {
			hint = 0;
			return c.keyframes.front().value;
		}
		if(t >= c.keyframes.back().time) {
			hint = c.keyframes.size() - 1;
			return c.keyframes.back().value;
		}

		// ヒント位置から探索スタート
		if(hint >= c.keyframes.size() || c.keyframes[hint].time > t) {
			hint = 0;
		}

		while(hint + 1 < c.keyframes.size() && t >= c.keyframes[hint + 1].time) {
			hint++;
		}

		size_t i0 = hint;
		size_t i1 = i0 + 1;

		if(i1 >= c.keyframes.size()) {
			return c.keyframes[i0].value;
		}

		float lT = (t - c.keyframes[i0].time) / (c.keyframes[i1].time - c.keyframes[i0].time);
		return CalyxEngine::Vector3::Lerp(c.keyframes[i0].value, c.keyframes[i1].value, lT);
	}

	void AnimationModel::SkinningStep() {
		if(!modelData_) return;

		Skeleton&	 skel		= modelData_->skeleton;
		const size_t jointCount = skel.joints.size();

		// キャッシュ初期化チェック
		auto checkCache = [&](AnimationState* st) {
			if(!st) return;
			if(st->hintTranslate.size() != jointCount) st->hintTranslate.assign(jointCount, 0);
			if(st->hintRotate.size() != jointCount) st->hintRotate.assign(jointCount, 0);
			if(st->hintScale.size() != jointCount) st->hintScale.assign(jointCount, 0);
		};
		checkCache(currentAnimation_);
		checkCache(nextAnimation_);

		auto blendOne = [&](AnimationState*		   st,
							size_t				   j,
							CalyxEngine::Quaternion& rot,
							CalyxEngine::Vector3&	   pos,
							CalyxEngine::Vector3&	   scl,
							float&				   wSum) {
			if(!st || st->weight <= 0.f) return;
			const NodeAnimation* node = st->animation.fastChannels[j];
			if(!node) return;

			// translate
			if(!node->translate.keyframes.empty()) {
				CalyxEngine::Vector3 t = CalculateValue(node->translate, st->currentTime, st->hintTranslate[j]);
				pos += (t - skel.joints[j].restTransform.translate) * st->weight;
			}
			// scale
			if(!node->scale.keyframes.empty()) {
				CalyxEngine::Vector3 s = CalculateValue(node->scale, st->currentTime, st->hintScale[j]);
				scl += (s - skel.joints[j].restTransform.scale) * st->weight;
			}
			// rotate
			if(!node->rotate.keyframes.empty()) {
				CalyxEngine::Quaternion q = CalculateValue(node->rotate, st->currentTime, st->hintRotate[j]);
				rot						= (wSum == 0.f) ? q : CalyxEngine::Quaternion::Slerp(rot, q, st->weight / (wSum + st->weight));
				wSum += st->weight;
			}
		};

		for(size_t j = 0; j < jointCount; ++j) {
			Joint&		joint = skel.joints[j];
			const auto& rest  = joint.restTransform;

			CalyxEngine::Quaternion R = rest.rotate;
			CalyxEngine::Vector3	  P = rest.translate;
			CalyxEngine::Vector3	  S = rest.scale;
			float				  w = 0.f;

			blendOne(currentAnimation_, j, R, P, S, w);
			blendOne(nextAnimation_, j, R, P, S, w);

			joint.transform.rotate	  = CalyxEngine::Quaternion::Normalize(R);
			joint.transform.translate = P;
			joint.transform.scale	  = S;

			// local → skeleton space
			joint.localMatrix = CalyxEngine::MakeAffineMatrix(S, R, P);
			joint.skeletonSpaceMatrix =
				joint.parent
					? (joint.localMatrix *
					   skel.joints[*joint.parent].skeletonSpaceMatrix)
					: joint.localMatrix;

			// パレット計算
			auto& dst = skinCluster_.mappedPalette[j];
			dst.skeletonSpaceMatrix =
				skinCluster_.inverseBindPoseMatrices[j] * joint.skeletonSpaceMatrix;
			dst.skeletonSpaceInverseTransposeMatrix =
				CalyxEngine::Matrix4x4::Transpose(
					CalyxEngine::Matrix4x4::Inverse(dst.skeletonSpaceMatrix));
		}
	}

	//-----------------------------------------------------------------------------
	// 毎フレームの更新
	//-----------------------------------------------------------------------------
	void AnimationModel::SkeletonUpdate() {
		// すべてのjointを更新
		for(Joint& joint : modelData_->skeleton.joints) {
			joint.localMatrix = CalyxEngine::MakeAffineMatrix(joint.transform.scale, joint.transform.rotate, joint.transform.translate);

			// 親の行列がある場合は、親の行列を掛け合わせる
			if(joint.parent) {
				joint.skeletonSpaceMatrix = joint.localMatrix * modelData_->skeleton.joints[*joint.parent].skeletonSpaceMatrix;
			} else {
				joint.skeletonSpaceMatrix = joint.localMatrix;
			}
		}
	}

	void AnimationModel::SkinClusterUpdate() {
		for(size_t jointIndex = 0; jointIndex < modelData_->skeleton.joints.size(); ++jointIndex) {
			CX_CHECK(jointIndex < skinCluster_.inverseBindPoseMatrices.size(), "Assertion failed");
			skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix =
				skinCluster_.inverseBindPoseMatrices[jointIndex] * modelData_->skeleton.joints[jointIndex].skeletonSpaceMatrix;
			skinCluster_.mappedPalette[jointIndex].skeletonSpaceInverseTransposeMatrix =
				CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(skinCluster_.mappedPalette[jointIndex].skeletonSpaceMatrix));
		}
	}

	void AnimationModel::DrawSkeleton() {
		modelData_->skeleton.Draw();
	}

	void AnimationModel::DrawSkeleton(const WorldTransform& transform) {
		if(!modelData_) return;
		CalyxEngine::Vector4 col = {jointHighlightCol_.x, jointHighlightCol_.y,
									jointHighlightCol_.z, jointHighlightCol_.w};
		modelData_->skeleton.Draw(transform.matrix.world, selectedJoint_, col);
	}

	std::string AnimationModel::GetCurrentAnimationName() const {
		if(currentAnimation_) {
			return currentAnimation_->name;
		} else {
			return "";
		}
	}

	void AnimationModel::OnModelLoaded() {
		// .png を優先的に使用するため、一時的にパスを書き換える
		std::string			  originalPath = modelData_->meshResource.Material().textureFilePath;
		std::filesystem::path p(originalPath);
		p.replace_extension(".png");
		modelData_->meshResource.Material().textureFilePath = p.string();

		BaseModel::OnModelLoaded();

		// 元に戻す（他のモデルへの影響を防ぐため）
		modelData_->meshResource.Material().textureFilePath = originalPath;

		ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();

		/* スキンクラスター確保 */
		skinCluster_ = CreateSkinCluster(device, modelData_->skeleton, *modelData_);
		/* restPose を Skeleton に保存 (ロード直後の Transform) */
		for(auto& j : modelData_->skeleton.joints) {
			j.restTransform = j.transform;
		}
		/* 高速テーブル */
		/* --- ここで全アニメの fastChannels を作る ------------------- */
		for(auto& [_, state] : animationStates_) {
			BuildFastChannels(state.animation);
		}
	}

	void AnimationModel::EnsureGpuSkinningResources(ID3D12Device* device) {
		if(gpuSkinningResourcesCreated_) return;
		if(!device || !modelData_) return;

		const UINT vertexCount = static_cast<UINT>(modelData_->meshResource.Vertices().size());
		if(vertexCount == 0) return;

		if(sourceVertexSrv_.cpu.ptr == 0) {
			sourceVertexSrv_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
		}
		D3D12_SHADER_RESOURCE_VIEW_DESC vertexSrv{};
		vertexSrv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		vertexSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		vertexSrv.Format = DXGI_FORMAT_UNKNOWN;
		vertexSrv.Buffer.FirstElement = 0;
		vertexSrv.Buffer.NumElements = vertexCount;
		vertexSrv.Buffer.StructureByteStride = sizeof(VertexPosUvN);
		vertexSrv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device->CreateShaderResourceView(
			modelData_->meshResource.VertexBuffer().GetResource().Get(),
			&vertexSrv,
			sourceVertexSrv_.cpu);

		if(influenceSrv_.cpu.ptr == 0) {
			influenceSrv_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
		}
		D3D12_SHADER_RESOURCE_VIEW_DESC influenceSrv{};
		influenceSrv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		influenceSrv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		influenceSrv.Format = DXGI_FORMAT_UNKNOWN;
		influenceSrv.Buffer.FirstElement = 0;
		influenceSrv.Buffer.NumElements = vertexCount;
		influenceSrv.Buffer.StructureByteStride = sizeof(VertexInfluence);
		influenceSrv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		device->CreateShaderResourceView(
			skinCluster_.influenceResource.Get(),
			&influenceSrv,
			influenceSrv_.cpu);

		skinnedVertexBuffer_.InitializeAsRW(device, vertexCount);
		skinnedVertexBuffer_.CreateUav(device);
		skinnedVertexState_ = D3D12_RESOURCE_STATE_COMMON;

		skinnedVertexBufferView_.BufferLocation = skinnedVertexBuffer_.GetResource()->GetGPUVirtualAddress();
		skinnedVertexBufferView_.SizeInBytes = sizeof(VertexPosUvN) * vertexCount;
		skinnedVertexBufferView_.StrideInBytes = sizeof(VertexPosUvN);

		gpuSkinningResourcesCreated_ = true;
	}

	void AnimationModel::DispatchSkinning(PipelineService* psoService, ID3D12GraphicsCommandList* cmdList) {
		if(!modelData_ || !psoService || !cmdList) return;
		ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
		EnsureGpuSkinningResources(device);
		if(!gpuSkinningResourcesCreated_) return;

		auto* resource = skinnedVertexBuffer_.GetResource().Get();
		if(skinnedVertexState_ != D3D12_RESOURCE_STATE_UNORDERED_ACCESS) {
			auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				resource,
				skinnedVertexState_,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			cmdList->ResourceBarrier(1, &barrier);
			skinnedVertexState_ = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		}

		const auto& ps = psoService->GetComputePipelineSet(PipelineTag::Compute::SkinningCompute);
		ps.SetCompute(cmdList);

		const UINT vertexCount = static_cast<UINT>(modelData_->meshResource.Vertices().size());
		cmdList->SetComputeRoot32BitConstant(0, vertexCount, 0);
		cmdList->SetComputeRootDescriptorTable(1, sourceVertexSrv_.gpu);
		cmdList->SetComputeRootDescriptorTable(2, influenceSrv_.gpu);
		cmdList->SetComputeRootDescriptorTable(3, skinCluster_.paletteSrvHandle.second);
		cmdList->SetComputeRootDescriptorTable(4, skinnedVertexBuffer_.GetGpuUavHandle());
		cmdList->Dispatch((vertexCount + 127u) / 128u, 1, 1);

		D3D12_RESOURCE_BARRIER barriers[2]{};
		barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barriers[0].UAV.pResource = resource;
		barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
			resource,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		cmdList->ResourceBarrier(2, barriers);

		skinnedVertexState_ =
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		skinnedVertexReady_ = true;
	}

	void AnimationModel::EnsureRaytracingBLAS(ID3D12Device5* device5, ID3D12GraphicsCommandList4* cmdList4) {
		if(!modelData_) return;
		if(!device5 || !cmdList4) return;
		if(!skinnedVertexReady_ || !skinnedVertexBuffer_.IsValid()) return;

		rayMesh_.BuildBLAS(
			device5,
			cmdList4,
			skinnedVertexBuffer_.GetResource()->GetGPUVirtualAddress(),
			static_cast<UINT>(modelData_->meshResource.Vertices().size()),
			sizeof(VertexPosUvN),
			modelData_->meshResource.IndexBuffer().GetResource()->GetGPUVirtualAddress(),
			static_cast<UINT>(modelData_->meshResource.Indices().size()));
		blasBuilt_ = true;
	}

	void AnimationModel::RegisterAnimation(
		int16_t							  animID,
		const std::string&				  animName,
		const std::optional<std::string>& fileName) {
		// 実際に使用するファイル名を決定
		std::string finalFile;

		if(fileName.has_value()) {
			// 呼び出し側がファイル名を指定した場合
			finalFile = fileName.value();
		} else {
			// 自動で animName + ".gltf"
			finalFile = animName + ".gltf";
		}

		// モデルにアニメを追加
		AddAnimation(animName, finalFile);

		// ID → 名前の対応を登録
		animIdTable_[animID] = animName;
	}

	void AnimationModel::Play(int16_t id, float blend) {
		auto it = animIdTable_.find(id);
		if(it == animIdTable_.end()) return;

		// 通常再生なのでワンショットフラグはオフ
		isOneShot_ = false;

		PlayAnimation(it->second, blend);
	}

	void AnimationModel::PlayOneShot(int16_t id, int16_t returnAnim, float blend) {
		auto it = animIdTable_.find(id);
		if(it == animIdTable_.end()) return;

		isOneShot_	   = true;
		oneShotReturn_ = returnAnim;

		// ワンショットはループさせない
		SetLoop(id, false);

		PlayAnimation(it->second, blend);
	}

	void AnimationModel::SetLoop(int16_t id, bool isLoop) {
		auto it = animIdTable_.find(id);
		if(it == animIdTable_.end()) return;

		const std::string& name = it->second;

		auto stIt = animationStates_.find(name);
		if(stIt == animationStates_.end()) return;

		stIt->second.loop = isLoop;
	}

	bool AnimationModel::IsAnimationFinished() const {
		if(!currentAnimation_) return false;

		if(currentAnimation_->loop)
			return false; // ループアニメは終わらない

		return currentAnimation_->currentTime >= currentAnimation_->animation.duration;
	}

	void AnimationModel::SetCommandPalletSrv(UINT rootParameterIndex, ID3D12GraphicsCommandList* cmdList) const {
		cmdList->SetGraphicsRootDescriptorTable(rootParameterIndex, skinCluster_.paletteSrvHandle.second);
	}

	void AnimationModel::BindVertexIndexBuffers(ID3D12GraphicsCommandList* cmdList) const {
		if(!modelData_) return;

		modelData_->meshResource.IndexBuffer().SetCommand(cmdList);
		if(skinnedVertexReady_) {
			cmdList->IASetVertexBuffers(0, 1, &skinnedVertexBufferView_);
			return;
		}

		vbvs_[0] = modelData_->meshResource.VertexBuffer().GetVertexBufferView();
		cmdList->IASetVertexBuffers(0, 1, vbvs_);
	}

	//-----------------------------------------------------------------------------
	// 描画
	//-----------------------------------------------------------------------------
	void AnimationModel::Draw([[maybe_unused]] const WorldTransform& transform) {
		// もしモデルデータが読み込まれていない場合は何もしない
		if(!modelData_) {
			return;
		}
		if(!handle_) {
			return;
		}

		ID3D12GraphicsCommandList* cmdList = GraphicsGroup::GetInstance()->GetCommandList().Get();
		ID3D12Device*			   device  = GraphicsGroup::GetInstance()->GetDevice().Get();

		std::vector<WorldTransform> singleTransform{transform};
		EnsureInstanceCapacity(device, 1);
		UploadInstanceMatrices(singleTransform);

		materialBuffer_.SetCommand(cmdList, 0);
		cmdList->SetGraphicsRootDescriptorTable(1, GetInstanceSrv());
		cmdList->SetGraphicsRootDescriptorTable(2, GetTexSrv());
		cmdList->SetGraphicsRootDescriptorTable(6, GetEnvMapSrv());
		SetCommandPalletSrv(7, cmdList);
		cmdList->SetGraphicsRootDescriptorTable(14, GetNormalMapSrv());

		// 頂点バッファ/インデックスバッファをセット
		BindVertexIndexBuffers(cmdList);

		cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		const auto& subMeshes = modelData_->meshResource.SubMeshes();
		if(subMeshes.empty()) {
			cmdList->DrawIndexedInstanced(UINT(modelData_->meshResource.Indices().size()), 1, 0, 0, 0);
		} else {
			for(const auto& subMesh : subMeshes) {
				cmdList->SetGraphicsRootDescriptorTable(2, GetTexSrv(subMesh.materialIndex));
				cmdList->SetGraphicsRootDescriptorTable(14, GetNormalMapSrv(subMesh.materialIndex));
				cmdList->DrawIndexedInstanced(subMesh.indexCount, 1, subMesh.indexStart, 0, 0);
			}
		}

		if(isDrawSkeleton_) {
			DrawSkeleton(transform);
		}
	}

	//-----------------------------------------------------------------------------
	// ImGui などUIの表示
	//-----------------------------------------------------------------------------
	void AnimationModel::ShowImGuiInterface() {
#if defined(_DEBUG) || defined(DEVELOP)
		GuiCmd::CheckBox("Draw Skeleton", isDrawSkeleton_);

		// ------ ジョイントリスト ---------------------------------
		if(ImGui::CollapsingHeader("Skeleton##header")) {
			// 名前配列を一度だけ作る
			static std::vector<const char*> jointNames;
			if(jointNames.empty() && modelData_) {
				jointNames.reserve(modelData_->skeleton.joints.size());
				for(auto& j : modelData_->skeleton.joints) {
					jointNames.push_back(j.name.c_str());
				}
			}

			if(ImGui::ListBox("Joints", &selectedJoint_,
							  jointNames.data(),
							  static_cast<int>(jointNames.size()), 10)) {
			}

			// 色を変える UI
			ImGui::ColorEdit4("Highlight", (float*)&jointHighlightCol_,
							  ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);
		}

		// 選択中ジョイントの情報表示
		if(selectedJoint_ >= 0 && modelData_) {
			const Joint& j = modelData_->skeleton.joints[selectedJoint_];
			ImGui::Text("Index: %d  Parent: %d", j.index,
						j.parent ? *j.parent : -1);
			ImGui::Text("Local Pos :  %.3f, %.3f, %.3f",
						j.transform.translate.x,
						j.transform.translate.y,
						j.transform.translate.z);
		}

#endif
	}

	//-----------------------------------------------------------------------------
	// バッファ生成/マッピング
	//-----------------------------------------------------------------------------
	void AnimationModel::Map() { MaterialBufferMap(); }

	void AnimationModel::CreateMaterialBuffer() {
		ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
		materialBuffer_.Initialize(device);
	}

	void AnimationModel::MaterialBufferMap() {
		TransferMaterial();
	}

	//-----------------------------------------------------------------------------
	// ノード名の取得
	//-----------------------------------------------------------------------------
	std::vector<std::string> AnimationModel::GetAnimationNodeNames() const {
		std::vector<std::string> names;
		if(!modelData_) return names;
		for(auto& pair : modelData_->animation.nodeAnimations) {
			names.push_back(pair.first);
		}
		return names;
	}

	std::vector<std::string> AnimationModel::GetJointNames() const {
		std::vector<std::string> names;
		if(!modelData_) return names;

		names.reserve(modelData_->skeleton.joints.size());
		for(const auto& joint : modelData_->skeleton.joints) {
			names.push_back(joint.name);
		}
		return names;
	}

	//-----------------------------------------------------------------------------
	// ジョイントの行列取得
	//-----------------------------------------------------------------------------
	std::optional<CalyxEngine::Matrix4x4> AnimationModel::GetJointMatrix(const std::string& name) const {
		if(!modelData_) return std::nullopt;

		auto it = modelData_->skeleton.jointMap.find(name);
		if(it == modelData_->skeleton.jointMap.end()) return std::nullopt;

		const Joint& j = modelData_->skeleton.joints[it->second];
		return j.skeletonSpaceMatrix;
	}

	bool AnimationModel::SetSelectedJointByName(const std::string& name) {
		if(!modelData_) return false;

		auto it = modelData_->skeleton.jointMap.find(name);
		if(it == modelData_->skeleton.jointMap.end()) return false;

		selectedJoint_ = it->second;
		return true;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE AnimationModel::GetJointMatrixSrv() const { return skinCluster_.paletteSrvHandle.second; }

	bool AnimationModel::HasSkinnedVertexBuffer() const { return skinnedVertexReady_; }

} // namespace CalyxEngine
