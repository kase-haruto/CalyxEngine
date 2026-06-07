#include "ModelRenderer.h"
#include <Engine/Foundation/Debug/CxAssert.h>

/* ========================================================================
/* include space
/* ===================================================================== */
#include "Engine/Foundation/Math/Matrix3x4.h"

#include "Engine/Graphics/Context/GraphicsGroup.h"
#include "Engine/Graphics/Shadow/ShadowMap/ShadowMapSystem.h"
#include <Engine/Assets/Animation/AnimationModel.h>
#include <Engine/Assets/Model/BaseModel.h>
#include <Engine/Graphics/Camera/3d/Camera3d.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Lighting/LightLibrary.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <cstring>

ModelRenderer::ModelRenderer() {
	// DXR (DirectX Raytracing) 対応デバイスかを確認し、対応している場合のみ RaytracingSystem を初期化
	Microsoft::WRL::ComPtr<ID3D12Device5> device5;
	GraphicsGroup::GetInstance()->GetDevice()->QueryInterface(IID_PPV_ARGS(&device5));

	if(device5) {
		// ID3D12Device5 が取得できた場合のみ Raytracing を有効化
		raytracingSystem_ = std::make_unique<CalyxEngine::RaytracingSystem>();
		raytracingSystem_->Initialize(device5.Get());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル登録（ビルボードモード付き）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterStatic(BaseModel* model, const WorldTransform& transform, BillboardMode billMode, SceneObject* owner) {
	InstanceStatic inst{};
	inst.tf		 = transform;
	inst.dirty	 = true;
	inst.visible = false;
	inst.mode	 = billMode;
	inst.owner	 = owner;
	staticModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アニメーションモデル登録
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::RegisterSkinned(CalyxEngine::AnimationModel* model, const WorldTransform& transform, SceneObject* owner) {
	InstanceSkinned inst{};
	inst.tf		 = transform;
	inst.dirty	 = true;
	inst.visible = false;
	inst.owner	 = owner;
	skinnedModels_[model].push_back(inst);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		クリア
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::Clear() {
	staticModels_.clear();
	skinnedModels_.clear();
	staticBatches_.clear();
	skinnedBatches_.clear();
	tempVisibleStatic_.clear();
	tempVisibleSkinned_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		フレーム開始
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BeginFrame() {
	// 前フレームの可視フラグをリセット（カリング結果は次の PreCullAndBatch で更新される）
	for(auto& insts : staticModels_ | std::views::values) {
		for(auto& inst : insts) {
			inst.visible = false;
		}
	}
	for(auto& insts : skinnedModels_ | std::views::values) {
		for(auto& inst : insts) {
			inst.visible = false;
		}
	}
	// バッチ情報をクリア（毎フレーム再構築）
	staticBatches_.clear();
	skinnedBatches_.clear();

	// モデルインスタンスは毎フレーム Register～ で再登録する方式のため全クリア
	staticModels_.clear();
	skinnedModels_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		Dirty マーク
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::MarkStaticDirty(BaseModel* model, size_t index) {
	auto it = staticModels_.find(model);
	if(it == staticModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

void ModelRenderer::MarkSkinnedDirty(CalyxEngine::AnimationModel* model, size_t index) {
	auto it = skinnedModels_.find(model);
	if(it == skinnedModels_.end()) return;
	if(index >= it->second.size()) return;
	it->second[index].dirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		視錐台カリング & バッチ化（PrePass）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::PreCullAndBatch(const Camera3d* camera, bool enableFrustumCulling) {

	// =========================================================
	// 静的モデル：視錐台カリングと World AABB の最新化
	// =========================================================
	for(auto& [model, insts] : staticModels_) {
		if(!model) continue;
		if(!model->GetModelData()) continue;
		if(!model->GetIsDrawEnable()) continue; // 描画無効のモデルはスキップ

		const AABB& localAABB = model->GetModelData()->localAABB;

		for(auto& inst : insts) {

			// ダーティフラグが立っている場合のみ World AABB を再計算（パフォーマンス最適化）
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}

			// -------------------------
			// MainPass：カメラ視錐台カリング
			// フラスタムカリングが無効の場合、またはカメラが存在しない場合は常に可視とする
			// -------------------------
			inst.visible = !enableFrustumCulling || !camera || camera->IsVisible(inst.worldAABB);
		}
	}

	// =========================================================
	// スキンメッシュモデル（Static と同じ視錐台カリング処理）
	// =========================================================
	for(auto& [model, insts] : skinnedModels_) {
		if(!model) continue;
		if(!model->GetModelData()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;

		for(auto& inst : insts) {

			// ダーティフラグが立っている場合のみ World AABB を再計算
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}

			// -------------------------
			// MainPass：カメラ視錐台カリング
			// -------------------------
			inst.visible = !enableFrustumCulling || !camera || camera->IsVisible(inst.worldAABB);
		}
	}

	// シャドウキャスター（シャドウマップ描画対象）の収集を行う
	CollectShadowCasters();

	// =========================================================
	// MainPass 用バッチ生成（可視インスタンスをパイプライン別にグループ化）
	// =========================================================
	BuildStaticBatches();
	BuildSkinnedBatches();
}

void ModelRenderer::BuildAllVisibleBatches() {
	for(auto& [model, insts] : staticModels_) {
		if(!model || !model->GetModelData() || !model->GetIsDrawEnable()) continue;
		for(auto& inst : insts) {
			inst.visible = true;
			if(inst.dirty) {
				inst.worldAABB = model->GetModelData()->localAABB.Transform(inst.tf.matrix.world);
				inst.dirty = false;
			}
		}
	}

	for(auto& [model, insts] : skinnedModels_) {
		if(!model || !model->GetModelData()) continue;
		for(auto& inst : insts) {
			inst.visible = true;
			if(inst.dirty) {
				inst.worldAABB = model->GetModelData()->localAABB.Transform(inst.tf.matrix.world);
				inst.dirty = false;
			}
		}
	}

	CollectShadowCasters();

	BuildStaticBatches();
	BuildSkinnedBatches();
}

void ModelRenderer::CollectShadowCasters() {
	// シャドウキャスターリストと Raytracing シーンをリセット
	staticVisibleForShadow_.clear();
	skinnedVisibleForShadow_.clear();
	raytracingScene_.Clear();
	hasSceneBounds_ = false;

	// シーン全体の AABB を累積拡張するラムダ（シャドウ計算のためのシーン境界を決定する）
	auto expandSceneBounds = [&](const AABB& aabb) {
		if(!hasSceneBounds_) {
			// 最初の AABB を基準にシーン境界を初期化
			sceneBounds_	= aabb;
			hasSceneBounds_ = true;
			return;
		}
		// 既存境界と Union を取って拡張
		sceneBounds_.min_ = CalyxEngine::Vector3::Min(sceneBounds_.min_, aabb.min_);
		sceneBounds_.max_ = CalyxEngine::Vector3::Max(sceneBounds_.max_, aabb.max_);
	};

	// --- 静的モデルのシャドウキャスター収集 ---
	for(auto& [model, insts] : staticModels_) {
		if(!model || !model->GetModelData() || !model->GetIsDrawEnable()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;
		for(auto& inst : insts) {
			// オーナーが CastShadow=false に設定されている場合はシャドウ対象外
			if(inst.owner && !inst.owner->IsCastShadow()) continue;
			// World AABB が古い場合は更新
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}
			staticVisibleForShadow_[model].push_back(inst.tf);
			expandSceneBounds(inst.worldAABB);
		}
	}

	// --- スキンメッシュモデルのシャドウキャスター収集 ---
	for(auto& [model, insts] : skinnedModels_) {
		if(!model || !model->GetModelData() || !model->GetIsDrawEnable()) continue;

		const AABB& localAABB = model->GetModelData()->localAABB;
		for(auto& inst : insts) {
			// オーナーが CastShadow=false に設定されている場合はシャドウ対象外
			if(inst.owner && !inst.owner->IsCastShadow()) continue;
			// World AABB が古い場合は更新
			if(inst.dirty) {
				inst.worldAABB = localAABB.Transform(inst.tf.matrix.world);
				inst.dirty	   = false;
			}
			skinnedVisibleForShadow_[model].push_back(inst.tf);
			expandSceneBounds(inst.worldAABB);
		}
	}
}

void ModelRenderer::BindRaytracingScene(ID3D12GraphicsCommandList* cmdList) const {
	// RaytracingSystem が未初期化（DXR 非対応デバイス）の場合は何もしない
	if(!raytracingSystem_) return;
	ID3D12Resource* tlas = raytracingSystem_->GetTLAS();
	if(!tlas) return; // TLAS が構築されていない場合もスキップ

	// ルートパラメータ slot 10 にTLASのGPUアドレスをバインド（シェーダー内で影判定に使用）
	cmdList->SetGraphicsRootShaderResourceView(
		10, // Space0, t3
		tlas->GetGPUVirtualAddress());
}

/////////////////////////////////////////////////////////////////////////////////////////
//		静的モデル・バッチ作成（BillboardParams も可視分だけ詰める）
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildStaticBatches() {
	for(auto& [model, insts] : staticModels_) {
		if(!model->GetModelData() || !model->GetIsDrawEnable()) continue;

		PipelineKey key{PipelineTag::Object::Object3d, model->GetBlendMode()};
		auto&		batch = staticBatches_[key];

		for(auto& inst : insts) {
			if(!inst.visible) continue;

			const bool cameraDitherEnabled = !inst.owner || inst.owner->IsCameraDitherEnabled();
			auto* item = FindCompatibleStaticBatch(batch, model, cameraDitherEnabled);
			if(!item) {
				StaticBatchItem newItem;
				newItem.model = model;
				newItem.cameraDitherEnabled = cameraDitherEnabled;
				batch.emplace_back(std::move(newItem));
				item = &batch.back();
			}

			item->transforms.push_back(inst.tf);

			GpuBillboardParams p{};
			p.mode = static_cast<uint32_t>(inst.mode);
			item->billboards.push_back(p);
		}
	}
}

// 同一のモデル・テクスチャ・マテリアル・ディザ設定を持つ互換バッチを検索して返す
// インスタンシング描画（DrawIndexedInstanced）のために同じ設定のインスタンスをひとつのバッチにまとめる
ModelRenderer::StaticBatchItem* ModelRenderer::FindCompatibleStaticBatch(StaticBatch& batch, BaseModel* model, bool cameraDitherEnabled) {
	if(!model || !model->GetModelData()) return nullptr;

	for(auto& item : batch) {
		BaseModel* base = item.model;
		if(!base || !base->GetModelData()) continue;
		if(item.cameraDitherEnabled != cameraDitherEnabled) continue; // カメラディザ設定が異なる場合は別バッチ
		if(base->GetModelData() != model->GetModelData()) continue;   // 異なるメッシュデータは別バッチ
		if(base->GetTexSrv().ptr != model->GetTexSrv().ptr) continue; // テクスチャSRVが異なれば別バッチ
		if(base->GetNormalMapSrv().ptr != model->GetNormalMapSrv().ptr) continue; // 法線マップが異なれば別バッチ
		if(base->GetEnvMapSrv().ptr != model->GetEnvMapSrv().ptr) continue;       // 環境マップが異なれば別バッチ
		if(base->UsesRuntimeMaterialGraph() != model->UsesRuntimeMaterialGraph()) continue; // マテリアルグラフ使用有無が異なれば別バッチ
		if(base->UsesRuntimeMaterialGraph() && base->GetMaterialGuid() != model->GetMaterialGuid()) continue; // マテリアルGUIDが異なれば別バッチ
		if(std::memcmp(&base->GetMaterialForBatch(), &model->GetMaterialForBatch(), sizeof(Material)) != 0) continue; // マテリアルパラメータが異なれば別バッチ

		return &item; // 全条件が一致 → 同一バッチにマージ可能
	}

	return nullptr; // 互換バッチが見つからなかった → 新規バッチ作成
}

/////////////////////////////////////////////////////////////////////////////////////////
//		スキンモデル・バッチ作成
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::BuildSkinnedBatches() {
	for(auto& [model, insts] : skinnedModels_) {
		if(!model->GetModelData()) continue;

		SkinnedBatchItem ditherOn;
		ditherOn.model = model;
		ditherOn.cameraDitherEnabled = true;
		SkinnedBatchItem ditherOff;
		ditherOff.model = model;
		ditherOff.cameraDitherEnabled = false;
		for(auto& inst : insts) {
			if(inst.visible) {
				const bool cameraDitherEnabled = !inst.owner || inst.owner->IsCameraDitherEnabled();
				(cameraDitherEnabled ? ditherOn.transforms : ditherOff.transforms).push_back(inst.tf);
			}
		}

		PipelineKey key{PipelineTag::Object::SkinningObject3D, model->GetBlendMode()};
		auto&		batch = skinnedBatches_[key];
		if(!ditherOn.transforms.empty()) {
			batch.emplace_back(std::move(ditherOn));
		}
		if(!ditherOff.transforms.empty()) {
			batch.emplace_back(std::move(ditherOff));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		一斉描画
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::DrawAll(ID3D12GraphicsCommandList*		cmdList,
							ID3D12Device*					device,
							IRenderTarget*					rt,
							PipelineService*				psoService,
							LightLibrary*					lightLibrary,
							CalyxEngine::ShadowMapSystem* shadowMapSystem) {
	(void)rt;

	// ============================================================
	// Phase 1: スキニング Compute Dispatch
	// スキンメッシュの頂点変換をGPUのコンピュートシェーダーで実行する
	// ============================================================
	{
		bool computeSet = false;
		for(auto& [model, insts] : skinnedModels_) {
			if(!model || !model->GetModelData() || insts.empty()) continue;
			// 最初のスキンモデルの描画時のみコンピュートパイプラインを設定（以降は共通）
			if(!computeSet) {
				const auto& ps = psoService->GetComputePipelineSet(PipelineTag::Compute::SkinningCompute);
				ps.SetCompute(cmdList);
				computeSet = true;
			}
			// スキニング行列をGPUに送信してスキニングコンピュートを実行
			model->DispatchSkinning(psoService, cmdList);
		}
	}

	// ============================================================
	// Phase 2: Raytracing TLAS の構築
	// DXR対応時のみ実行。影判定用のTop Level Acceleration Structureを再構築する
	// ============================================================
	if(raytracingSystem_) {
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> cmd4;
		Microsoft::WRL::ComPtr<ID3D12Device5>			   device5;

		if(SUCCEEDED(cmdList->QueryInterface(IID_PPV_ARGS(&cmd4))) &&
		   SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device5)))) {

			raytracingScene_.Clear();

			// 静的モデルの BLAS（Bottom Level AS）を確保してTLASインスタンスとして登録
			for(auto& [model, transforms] : staticVisibleForShadow_) {
				if(!model->GetModelData()) continue;

				model->EnsureRaytracingBLAS(device5.Get(), cmd4.Get());

				if(model->HasBLAS()) {
					for(const auto& tf : transforms) {
						raytracingScene_.AddInstance(
							CalyxEngine::Matrix3x4::ToMatrix3x4(tf.matrix.world),
							model->GetBLAS(),
							0);
					}
				}
			}

			// スキンメッシュモデルの BLAS を確保してTLASインスタンスとして登録
			for(auto& [model, transforms] : skinnedVisibleForShadow_) {
				if(!model->GetModelData()) continue;
				model->EnsureRaytracingBLAS(device5.Get(), cmd4.Get());

				if(model->HasBLAS()) {
					for(const auto& tf : transforms) {
						raytracingScene_.AddInstance(
							CalyxEngine::Matrix3x4::ToMatrix3x4(tf.matrix.world),
							model->GetBLAS(),
							0);
					}
				}
			}

			// TLAS のアップロードと構築
			raytracingScene_.EnsureBuffer(device);
			raytracingScene_.Upload();

			// スキニングコンピュートとのGPU同期用グローバルUAVバリア
			D3D12_RESOURCE_BARRIER uav = {};
			uav.Type				   = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uav.UAV.pResource		   = nullptr; // Global UAV barrier（すべてのUAVを対象とする）
			cmdList->ResourceBarrier(1, &uav);

			// TLAS のビルドコマンドを発行
			raytracingSystem_->BuildTLAS(cmd4.Get(), raytracingScene_);
		}
	}

	psoService->ResetState();

	// ============================================================
	// Phase 3: 静的モデル（スタティックメッシュ）描画
	// パイプラインキーごとにバッチを処理し、インスタンシング描画を行う
	// ============================================================
	{
		PipelineKey lastKey{};
		bool		hasLast = false;
		bool		usingGeneratedPipeline = false;

		// 毎バッチで繰り返すリソースバインド処理をラムダにまとめる
		auto bindObject3DPassResources = [&]() -> bool {
			// シャドウマップをメインパス用にバインド
			if(shadowMapSystem) {
				shadowMapSystem->BindForMainPass(cmdList);
			}

			// Raytracing シーン（TLAS）をシェーダーにバインド
			BindRaytracingScene(cmdList);

			// アクティブカメラの定数バッファをバインド（VP行列等）
			if(auto* cam = CameraManager::GetActive()) {
				cam->SetCommand(cmdList, PipelineType::Object3D);
			} else {
				return false; // カメラが存在しない場合はこのバッチをスキップ
			}

			// ライト情報をバインド
			lightLibrary->SetCommand(cmdList, PipelineType::Object3D);
			return true;
		};

		for(auto& [key, batch] : staticBatches_) {
			if(batch.empty()) continue;

			// パイプラインキーが変わった場合のみパイプラインを切り替える（不要な切り替えを省く）
			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				if(!bindObject3DPassResources()) continue;

				lastKey = key;
				hasLast = true;
				usingGeneratedPipeline = false;
			}

			for(auto& item : batch) {
				BaseModel* model   = item.model;
				auto&	   visible = item.transforms;
				if(!model || visible.empty()) continue;

				// マテリアルグラフを使用するモデルは専用のPSOを動的に取得して差し替える
				if(model->UsesRuntimeMaterialGraph()) {
					if(auto material = model->GetMaterialAsset()) {
						CalyxEngine::MaterialGraphRuntimeShader shader = runtimeMaterialShaderCache_.GetOrCompileObject3DPixelShader(*material);
						if(shader.pixelShader) {
							// 動的コンパイル済みシェーダーを使うカスタムパイプラインを設定
							const auto generatedSet = psoService->GetGeneratedMaterialObjectPipelineSet(model->GetBlendMode(), shader.pixelShader, shader.hash);
							psoService->SetCommand(generatedSet, cmdList);
							if(!bindObject3DPassResources()) continue;
							usingGeneratedPipeline = true;
						} else if(usingGeneratedPipeline) {
							// シェーダーコンパイル未完了の場合は標準パイプラインに戻す
							const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
							psoService->SetCommand(ps, cmdList);
							if(!bindObject3DPassResources()) continue;
							usingGeneratedPipeline = false;
						}
					}
				} else if(usingGeneratedPipeline) {
					// 直前が動的パイプラインで、今回は通常モデル → 標準パイプラインに戻す
					const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
					psoService->SetCommand(ps, cmdList);
					if(!bindObject3DPassResources()) continue;
					usingGeneratedPipeline = false;
				}

				// 描画インスタンス数を確認（ビルボードパラメータ数と一致する必要がある）
				const UINT need = static_cast<UINT>(item.billboards.size());
				if(need == 0) continue;
				CX_CHECK(item.transforms.size() == item.billboards.size(), "Assertion failed");

				// カメラディザリングのon/offフラグをシェーダーに設定
				const uint32_t objectDitherEnabled = item.cameraDitherEnabled ? 1u : 0u;
				cmdList->SetGraphicsRoot32BitConstants(13, 1, &objectDitherEnabled, 0);

				// ビルボードパラメータ（モード等）をGPU構造化バッファにアップロード
				model->EnsureBillboardCapacity(device, need);
				model->UploadBillboardParams(item.billboards);
				cmdList->SetGraphicsRootDescriptorTable(7, model->GetBillboardSrv());

				// インスタンスのワールド変換行列をGPU構造化バッファにアップロード
				model->EnsureInstanceCapacity(device, need);
				model->UploadInstanceMatrices(visible);
				cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());

				// マテリアルパラメータをGPUへ転送・バインド
				if(model->UsesRuntimeMaterialGraph()) {
					model->TransferMaterial();
				}
				model->BindMaterialCB(cmdList);
				cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());            // アルベドテクスチャ
				cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(0)); // マテリアルグラフ用テクスチャ
				cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());          // 環境マップ
				cmdList->SetGraphicsRootDescriptorTable(14, model->GetNormalMapSrv());      // 法線マップ

				// プリミティブトポロジーとVB/IBをバインドして描画コマンドを発行
				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				model->BindVertexIndexBuffers(cmdList);

				const auto& meshResource = model->GetModelData()->meshResource;
				const auto& subMeshes = meshResource.SubMeshes();
				if(subMeshes.empty()) {
					// サブメッシュ分割なし → 全インデックスを一括描画
					const UINT indexCount = static_cast<UINT>(meshResource.Indices().size());
					cmdList->DrawIndexedInstanced(indexCount, need, 0, 0, 0);
				} else {
					// サブメッシュごとにマテリアルを差し替えながら描画（マルチマテリアル対応）
					for(const auto& subMesh : subMeshes) {
						cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv(subMesh.materialIndex));
						cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(subMesh.materialIndex));
						cmdList->SetGraphicsRootDescriptorTable(14, model->GetNormalMapSrv(subMesh.materialIndex));
						cmdList->DrawIndexedInstanced(subMesh.indexCount, need, subMesh.indexStart, 0, 0);
					}
				}
			}
		}
	}

	//------------------------------------------------------------
	// スキンメッシュ
	//------------------------------------------------------------
	{
		PipelineKey lastKey{};
		bool		hasLast = false;
		bool		usingGeneratedPipeline = false;

		for(auto& [key, batch] : skinnedBatches_) {
			if(batch.empty()) continue;

			if(!hasLast || !(key == lastKey)) {
				const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
				psoService->SetCommand(ps, cmdList);

				if(shadowMapSystem) {
					shadowMapSystem->BindForMainPass(cmdList);
				}

				BindRaytracingScene(cmdList);

				if(auto* cam = CameraManager::GetActive()) {
					cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
				} else {
					// 判定漏れ防止
					continue;
				}

				lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);

				lastKey = key;
				hasLast = true;
				usingGeneratedPipeline = false;
			}

			for(auto& item : batch) {
				auto* model = item.model;
				auto& visible = item.transforms;
				if(!model || visible.empty()) continue;

				if(model->UsesRuntimeMaterialGraph()) {
					if(auto material = model->GetMaterialAsset()) {
						CalyxEngine::MaterialGraphRuntimeShader shader = runtimeMaterialShaderCache_.GetOrCompileObject3DPixelShader(*material);
						if(shader.pixelShader) {
							const auto generatedSet = psoService->GetGeneratedMaterialSkinnedPipelineSet(model->GetBlendMode(), shader.pixelShader, shader.hash);
							psoService->SetCommand(generatedSet, cmdList);
							if(shadowMapSystem) {
								shadowMapSystem->BindForMainPass(cmdList);
							}
							BindRaytracingScene(cmdList);
							if(auto* cam = CameraManager::GetActive()) {
								cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
							} else {
								continue;
							}
							lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
							usingGeneratedPipeline = true;
						} else if(usingGeneratedPipeline) {
							const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
							psoService->SetCommand(ps, cmdList);
							if(shadowMapSystem) {
								shadowMapSystem->BindForMainPass(cmdList);
							}
							BindRaytracingScene(cmdList);
							if(auto* cam = CameraManager::GetActive()) {
								cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
							} else {
								continue;
							}
							lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
							usingGeneratedPipeline = false;
						}
					}
				} else if(usingGeneratedPipeline) {
					const auto ps = psoService->GetPipelineSet(key.tag, key.blend);
					psoService->SetCommand(ps, cmdList);
					if(shadowMapSystem) {
						shadowMapSystem->BindForMainPass(cmdList);
					}
					BindRaytracingScene(cmdList);
					if(auto* cam = CameraManager::GetActive()) {
						cam->SetCommand(cmdList, PipelineType::SkinningObject3D);
					} else {
						continue;
					}
					lightLibrary->SetCommand(cmdList, PipelineType::SkinningObject3D);
					usingGeneratedPipeline = false;
				}

				const UINT need = static_cast<UINT>(visible.size());
				const uint32_t objectDitherEnabled = item.cameraDitherEnabled ? 1u : 0u;
				cmdList->SetGraphicsRoot32BitConstants(13, 1, &objectDitherEnabled, 0);
				model->EnsureInstanceCapacity(device, need);
				model->UploadInstanceMatrices(visible);
				cmdList->SetGraphicsRootDescriptorTable(1, model->GetInstanceSrv());

				if(model->UsesRuntimeMaterialGraph()) {
					model->TransferMaterial();
				}
				model->BindMaterialCB(cmdList);
				cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv());
				cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(0));
				cmdList->SetGraphicsRootDescriptorTable(6, model->GetEnvMapSrv());
				cmdList->SetGraphicsRootDescriptorTable(14, model->GetNormalMapSrv());
				model->SetCommandPalletSrv(7, cmdList);

				cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				model->BindVertexIndexBuffers(cmdList);

				const auto& meshResource = model->GetModelData()->meshResource;
				const auto& subMeshes = meshResource.SubMeshes();
				if(subMeshes.empty()) {
					const UINT indexCount = static_cast<UINT>(meshResource.Indices().size());
					cmdList->DrawIndexedInstanced(indexCount, need, 0, 0, 0);
				} else {
					for(const auto& subMesh : subMeshes) {
						cmdList->SetGraphicsRootDescriptorTable(2, model->GetTexSrv(subMesh.materialIndex));
						cmdList->SetGraphicsRootDescriptorTable(12, model->GetMaterialGraphTextureSrvTable(subMesh.materialIndex));
						cmdList->SetGraphicsRootDescriptorTable(14, model->GetNormalMapSrv(subMesh.materialIndex));
						cmdList->DrawIndexedInstanced(subMesh.indexCount, need, subMesh.indexStart, 0, 0);
					}
				}
			}
		}
	}

}

/////////////////////////////////////////////////////////////////////////////////////////
//		Picking / Outline / IDPass 用 可視リスト収集
/////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::CollectVisibleStatic(std::vector<RenderInstance>& out) const {
	for(const auto& [model, insts] : staticModels_) {
		if(!model) continue;

		for(const auto& inst : insts) {
			if(!inst.visible) continue;
			if(!inst.owner) continue;

			out.push_back(RenderInstance{
				model,
				&inst.tf,
				inst.owner,
				inst.mode});
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////
//		Picking / Outline / IDPass 用 可視リスト収集
///////////////////////////////////////////////////////////////////////////////////////////
void ModelRenderer::CollectVisibleSkinned(std::vector<RenderInstance>& out) const {
	for(const auto& [model, insts] : skinnedModels_) {
		if(!model) continue;

		for(const auto& inst : insts) {
			if(!inst.visible) continue;
			if(!inst.owner) continue;

			out.push_back(RenderInstance{
				model,
				&inst.tf,
				inst.owner,
				BillboardMode::None});
		}
	}
}
