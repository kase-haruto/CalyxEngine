#include "AssetPreviewManager.h"

#include <Data/Engine/Prefab/Serializer/PrefabSerializer.h>
#include <Engine/Assets/Database/AssetDatabase.h>
#include <Engine/Assets/Manager/AssetManager.h>
#include <Engine/Assets/Model/Model.h>
#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <Engine/Foundation/Math/MathUtil.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Graphics/Camera/3d/DebugCamera.h>
#include <Engine/Graphics/Camera/Manager/CameraManager.h>
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>
#include <Engine/Objects/Event/BaseEventObject.h>
#include <Engine/Renderer/Model/ModelRenderer.h>
#include <Engine/Scene/Context/SceneContext.h>

#include <array>
#include <cmath>
#include <system_error>

namespace CalyxEngine {
	namespace {
		///////////////////////////////////////////////////////////////////////////////
		// 指定された SceneObject と、その子オブジェクトを再帰的に収集する
		// 階層構造を持つオブジェクトをプレビュー描画対象としてまとめるために使用する
		///////////////////////////////////////////////////////////////////////////////
		void CollectObjectsRecursive(SceneObject* object, std::vector<SceneObject*>& out) {
			// null の場合は収集対象がないため終了する
			if(!object) return;

			// 自分自身を登録してから、子階層を順に辿る
			out.push_back(object);
			for(const auto& child : object->GetChildren()) {
				CollectObjectsRecursive(child.get(), out);
			}
		}

		///////////////////////////////////////////////////////////////////////////////
		// 2つの AABB を結合し、両方を含む AABB を作成する
		// 複数オブジェクト全体を収めるカメラ位置を決めるために使用する
		///////////////////////////////////////////////////////////////////////////////
		AABB MergeAabb(const AABB& a, const AABB& b) {
			return AABB{
				{
					(std::min)(a.min_.x, b.min_.x),
					(std::min)(a.min_.y, b.min_.y),
					(std::min)(a.min_.z, b.min_.z),
				},
				{
					(std::max)(a.max_.x, b.max_.x),
					(std::max)(a.max_.y, b.max_.y),
					(std::max)(a.max_.z, b.max_.z),
				}};
		}

		///////////////////////////////////////////////////////////////////////////////
		// モデル単体プレビュー用の基本 Transform を作成する
		// 原点・等倍・回転なしで配置し、カメラ側で見え方を調整する
		///////////////////////////////////////////////////////////////////////////////
		WorldTransform MakePreviewTransform() {
			WorldTransform transform;

			// プレビューではモデルをそのまま原点に配置する
			transform.scale		   = {1.0f, 1.0f, 1.0f};
			transform.rotation	   = CalyxEngine::Quaternion::MakeIdentity();
			transform.translation  = CalyxEngine::Vector3::Zero();
			// scale / rotation / translation からワールド行列を作成する
			transform.matrix.world = CalyxEngine::MakeAffineMatrix(
				transform.scale,
				transform.rotation,
				transform.translation);
			// ライティング計算で使用する逆転置行列も更新しておく
			transform.matrix.WorldInverseTranspose =
				CalyxEngine::Matrix4x4::Transpose(CalyxEngine::Matrix4x4::Inverse(transform.matrix.world));
			return transform;
		}
	} // namespace

	AssetPreviewManager AssetPreviewManager::instance_;

	///////////////////////////////////////////////////////////////////////////////
	// AssetPreviewManager のシングルトンインスタンスを取得する
	///////////////////////////////////////////////////////////////////////////////
	AssetPreviewManager* AssetPreviewManager::GetInstance() {
		return &instance_;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 指定されたアセット種別がプレビュー生成に対応しているか判定する
	// 現在は Model のみを対象としている
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::IsPreviewSupported(AssetType type) const {
		return type == AssetType::Model;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 指定されたアセットのプレビュー生成を要求する
	// 同じ更新状態のプレビューが存在する場合は、重複してキューに積まない
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::Request(const AssetRecord& record) {
		// GUID が無効、または未対応のアセット種別なら処理しない
		if(!record.guid.isValid() || !IsPreviewSupported(record.type)) return;

		// GUID ごとのプレビュー状態を取得する。存在しない場合は新規作成される
		auto& entry = entries_[record.guid];
		// 同じ更新日時・同じ種別で生成済みなら再生成しない
		if(entry.state == State::Ready && entry.lastWrite == record.lastWrite && entry.type == record.type) return;
		// すでに生成待ちならキューへの重複登録を避ける
		if(entry.state == State::Queued && entry.lastWrite == record.lastWrite && entry.type == record.type) return;
		// 同じ状態で一度失敗している場合は、無限に再試行しない
		if(entry.state == State::Failed && entry.lastWrite == record.lastWrite && entry.type == record.type) return;

		// 新しく生成対象にするため、古いプレビュー情報を初期化する
		entry.state				 = State::Queued;
		entry.texture			 = nullptr;
		entry.lastWrite			 = record.lastWrite;
		entry.type				 = record.type;
		entry.modelLoadRequested = false;
		// フレーム内で分割処理できるよう、GUID だけをキューに積む
		queue_.push_back(record.guid);
	}

	///////////////////////////////////////////////////////////////////////////////
	// 指定された GUID のプレビューキャッシュを無効化する
	// アセット更新・削除時に古いプレビューを破棄するために使用する
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::Invalidate(const Guid& guid) {
		if(!guid.isValid()) return;
		auto it = entries_.find(guid);
		if(it == entries_.end()) return;
		// GPU がまだ参照している可能性があるため、リソースは退避してから削除する
		RetireEntryResources(it->second);
		entries_.erase(it);
	}

	///////////////////////////////////////////////////////////////////////////////
	// すべてのプレビューキャッシュと生成キューを無効化する
	// アセットデータベースの再読み込み時などに使用する
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::InvalidateAll() {
		// 各エントリが持つ GPU リソースを安全に退避する
		for(auto& [guid, entry] : entries_) {
			(void)guid;
			// 既存のキャッシュリソースがあれば、GPU 使用中の可能性を考慮して退避する
		// 既存の描画キャッシュがあれば退避し、サイドカー画像の TextureID に差し替える
		RetireEntryResources(entry);
		}
		entries_.clear();
		queue_.clear();
	}

	///////////////////////////////////////////////////////////////////////////////
	// AssetPreviewManager が保持しているリソースを解放する
	// エンジン終了時やエディタ終了時に呼び出す
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::Shutdown() {
		// 生成待ちキューとキャッシュ状態をすべて破棄する
		// 生成待ちやキャッシュ情報を破棄する
		queue_.clear();
		entries_.clear();
		// フレーム跨ぎで保持していたオブジェクト・モデル・退避リソースを解放する
		// 前フレームでプレビュー描画のために延命していたオブジェクトを解放する
		retainedFrameObjects_.clear();
		// モデル単体プレビューで延命していた Model を解放する
		retainedFrameModels_.clear();
		// 退避していた GPU リソースをここで解放可能にする
		retiredFrameResources_.clear();
		modelRenderer_.reset();
		previewContext_.reset();
	}

	///////////////////////////////////////////////////////////////////////////////
	// 指定アセットのプレビュー TextureID を取得する
	// 未生成の場合は生成要求を出し、代わりに fallback を返す
	///////////////////////////////////////////////////////////////////////////////
	AssetPreviewManager::PreviewResult AssetPreviewManager::GetPreview(const AssetRecord& record, ImTextureID fallback) {
		// 未対応アセットの場合は、AssetRecord 側のプレビュー画像または fallback を返す
		if(!IsPreviewSupported(record.type)) {
			return PreviewResult{record.previewTex ? record.previewTex : fallback, record.previewTex != nullptr};
		}

		// 対応アセットの場合は、必要に応じてプレビュー生成を要求する
		Request(record);

		// 生成済みのプレビューがあれば、それを返す
		auto it = entries_.find(record.guid);
		if(it != entries_.end() && it->second.state == State::Ready && it->second.texture) {
			return PreviewResult{it->second.texture, true};
		}

		// まだ生成できていない場合は fallback を表示する
		return PreviewResult{fallback, false};
	}

	///////////////////////////////////////////////////////////////////////////////
	// プレビュー生成キューを処理する
	// 描画コマンドを必要としないサイドカープレビュー読み込みを主に扱う
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::ProcessQueue(int maxItemsPerFrame) {
		// 1フレームあたりの処理数が 0 以下なら何もしない
		if(maxItemsPerFrame <= 0) return;

		// GUID から最新の AssetRecord を取得するため、AssetDatabase を参照する
		// TextureManager で読み込むため、AssetDatabase のルートからの相対パスへ変換する
		auto* db = AssetDatabase::GetInstance();
		if(!db) return;

		int processed = 0;
		// フレームごとの処理数を制限し、エディタの負荷を分散する
		while(processed < maxItemsPerFrame && !queue_.empty()) {
			// キューの先頭から処理対象の GUID を取り出す
			// キューの先頭から処理対象の GUID を取り出す
			const Guid guid = queue_.front();
			queue_.pop_front();

			// エントリが存在しない、またはすでに別状態なら処理しない
			// エントリが存在しない、またはすでに別状態なら処理しない
			auto it = entries_.find(guid);
			if(it == entries_.end() || it->second.state != State::Queued) continue;

			// 最新の AssetRecord を取得し、削除済み・未対応の場合はエントリを破棄する
			// 最新の AssetRecord を取得し、削除済み・未対応の場合はエントリを破棄する
			const AssetRecord* record = db->Get(guid);
			if(!record || !IsPreviewSupported(record->type)) {
				entries_.erase(guid);
				continue;
			}

			// キュー登録後にアセットが更新されていた場合は、新しい情報で再要求する
			// キュー登録後にアセットが更新されていた場合は、新しい情報で再要求する
			if(record->lastWrite != it->second.lastWrite || record->type != it->second.type) {
				Request(*record);
				continue;
			}

			// サイドカー画像などから生成できれば Ready、できなければ Queued のままにする
			it->second.state = TryGeneratePreview(*record, it->second) ? State::Ready : State::Queued;
			++processed;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// 描画コマンドを使用してプレビュー生成キューを処理する
	// モデルのロード待ち、RenderTarget への描画、キャッシュへのコピーを行う
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::ProcessRenderQueue(ID3D12GraphicsCommandList* cmdList,
												 PipelineService*			pso,
												 IRenderTarget*				renderTarget,
												 int						maxItemsPerFrame) {
		// 描画に必要なオブジェクトが揃っていない場合は処理しない
		if(!cmdList || !pso || !renderTarget || maxItemsPerFrame <= 0) return;

		auto* db = AssetDatabase::GetInstance();
		if(!db) return;

		int processed = 0;
		// 描画を伴うため、1フレームで処理する数を制限する
		while(processed < maxItemsPerFrame && !queue_.empty()) {
			const Guid guid = queue_.front();
			queue_.pop_front();

			auto it = entries_.find(guid);
			if(it == entries_.end() || it->second.state != State::Queued) continue;

			const AssetRecord* record = db->Get(guid);
			if(!record || !IsPreviewSupported(record->type)) {
				entries_.erase(guid);
				continue;
			}

			if(record->lastWrite != it->second.lastWrite || record->type != it->second.type) {
				Request(*record);
				continue;
			}

			// モデルがまだロードされていない場合は、先にロード要求を出す
			if(record->type == AssetType::Model) {
				auto*			  assetManager = AssetManager::GetInstance();
				auto*			  modelManager = assetManager ? assetManager->GetModelManager() : nullptr;
				const std::string modelName	   = record->sourcePath.filename().string();
				// 非同期ロード中の場合は、完了するまでキューへ戻す
				if(modelManager && !modelManager->IsModelLoaded(modelName)) {
					// 初回のみロード要求を出し、以降は完了確認だけ行う
					if(!it->second.modelLoadRequested) {
						modelManager->LoadModel(modelName);
						it->second.modelLoadRequested = true;
					}
					// ロードタスクを進め、まだ完了していなければ次フレーム以降に再試行する
					modelManager->ProcessLoadingTasks();
					if(!modelManager->IsModelLoaded(modelName)) {
						it->second.state = State::Queued;
						queue_.push_back(guid);
						++processed;
						continue;
					}
				}
			}

			// サイドカー画像を優先し、なければモデルを実際に描画してプレビューを作成する
			if(TryGeneratePreview(*record, it->second) ||
			   TryRenderModelPreview(*record, it->second, cmdList, pso, renderTarget)) {
				it->second.state = State::Ready;
			} else {
				it->second.state = State::Failed;
			}
			++processed;
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// 1フレームだけ保持していたプレビュー用リソースを解放する
	// GPU が参照中の可能性があるリソースを即時破棄しないための退避領域をクリアする
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::ReleaseFrameResources() {
		retainedFrameObjects_.clear();
		retainedFrameModels_.clear();
		retiredFrameResources_.clear();
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセットのプレビュー生成を試みる
	// 現在はサイドカー画像の読み込みを優先し、実描画キャプチャは別処理で行う
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::TryGeneratePreview(const AssetRecord& record, Entry& entry) {
		// アセット本体とは別に用意されたプレビュー画像があれば、それを優先する
		if(TryLoadSidecarPreview(record, entry)) return true;

		// Runtime render capture is intentionally centralized here:
		// render into one reusable OffscreenRenderTarget, copy it into a
		// per-asset texture or atlas slot, then store its SRV in entry.texture.
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセットからプレビュー用オブジェクトを作成し、RenderTarget に描画する
	// Prefab など、複数オブジェクトを含むプレビュー生成に使用する
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::TryRenderPreview(const AssetRecord&		  record,
											   Entry&					  entry,
											   ID3D12GraphicsCommandList* cmdList,
											   PipelineService*			  pso,
											   IRenderTarget*			  renderTarget) {
		// プレビュー専用の SceneContext / Renderer を準備できなければ失敗とする
		// プレビュー専用の SceneContext / Renderer を準備できなければ失敗とする
		if(!EnsurePreviewContext()) return false;

		// 現在の SceneContext を退避し、プレビュー専用 Context を有効化する
		// 現在の SceneContext を退避し、プレビュー専用 Context を有効化する
		SceneContext* previous = SceneContext::Current();
		previewContext_->MakeCurrent();

		std::vector<std::shared_ptr<SceneObject>> objects;
		// アセット種別に応じて、描画用の一時オブジェクトを生成する
		try {
			objects = CreatePreviewObjects(record);
		} catch(const std::exception& e) {
			const std::string message = "Asset preview failed: " + record.sourcePath.filename().string() + " / " + e.what();
			CX_WARN(message.c_str());
			if(previous) previous->MakeCurrent();
			return false;
		} catch(...) {
			const std::string message = "Asset preview failed: " + record.sourcePath.filename().string();
			CX_WARN(message.c_str());
			if(previous) previous->MakeCurrent();
			return false;
		}
		// 描画対象が生成できなかった場合は、Context を戻して失敗とする
		if(objects.empty()) {
			if(previous) previous->MakeCurrent();
			return false;
		}

		// 生成したオブジェクト全体が収まるようにカメラとライト情報を更新する
		ConfigureCamera(objects);
		previewContext_->GetCameraMgr()->TransferToGPU();
		previewContext_->GetLightLibrary()->CyncGpu();

		// プレビュー用 RenderTarget をクリアして、描画先として設定する
		// プレビュー用 RenderTarget をクリアして、描画先として設定する
		renderTarget->Clear(cmdList);
		renderTarget->SetRenderTarget(cmdList);

		// 一時オブジェクトを走査し、描画対象として ModelRenderer に登録する
		modelRenderer_->BeginFrame();
		for(const auto& root : objects) {
			std::vector<SceneObject*> flattened;
			CollectObjectsRecursive(root.get(), flattened);
			for(SceneObject* object : flattened) {
				if(object) object->AlwaysUpdate(0.0f);
				RegisterPreviewObject(object);
			}
		}

		// 有効なカメラがあればカリングとバッチ構築を行う
		// 有効なカメラがあればカリングとバッチ構築を行う
		if(auto* camera = dynamic_cast<Camera3d*>(CameraManager::GetActive())) {
			modelRenderer_->PreCullAndBatch(camera, false);
		} else {
			modelRenderer_->BuildAllVisibleBatches();
		}

		// 登録されたモデルを RenderTarget に描画する
		// 登録されたモデルを RenderTarget に描画する
		modelRenderer_->DrawAll(cmdList,
								GraphicsGroup::GetInstance()->GetDevice().Get(),
								renderTarget,
								pso,
								previewContext_->GetLightLibrary(),
								nullptr);

		// 描画結果をアセットごとのキャッシュテクスチャへコピーする
		// 描画結果をアセットごとのキャッシュテクスチャへコピーする
		const bool copied = CopyRenderTargetToCache(cmdList, renderTarget, entry);
		// GPU コマンドが完了するまで、描画に使った一時オブジェクトを延命する
		retainedFrameObjects_.insert(retainedFrameObjects_.end(), objects.begin(), objects.end());
		if(previous) previous->MakeCurrent();
		return copied;
	}

	///////////////////////////////////////////////////////////////////////////////
	// モデルアセットを直接 RenderTarget に描画してプレビューを生成する
	// モデルの AABB から中心と半径を求め、カメラ距離を自動調整する
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::TryRenderModelPreview(const AssetRecord&		   record,
													Entry&					   entry,
													ID3D12GraphicsCommandList* cmdList,
													PipelineService*		   pso,
													IRenderTarget*			   renderTarget) {
		// モデル以外はこの経路では処理しない
		if(record.type != AssetType::Model) return false;
		if(!EnsurePreviewContext()) return false;

		// AssetRecord のファイル名から Model を生成する
		const std::string modelName = record.sourcePath.filename().string();
		auto			  model		= std::make_shared<Model>(modelName);
		model->Update(0.0f);

		// モデルデータが取得できない場合はプレビューを生成できない
		if(!model->GetModelData()) return false;

		// AABB からモデルの中心位置と半径を求め、カメラ距離を決める
		const AABB&	  bounds  = model->GetModelData()->localAABB;
		Vector3		  center  = (bounds.min_ + bounds.max_) * 0.5f;
		const Vector3 extents = (bounds.max_ - bounds.min_) * 0.5f;
		const float	  radius  = (std::max)(0.75f, extents.Length());

		SceneContext* previous = SceneContext::Current();
		previewContext_->MakeCurrent();

		// プレビューは固定サイズの DebugCamera で描画する
		// プレビューは固定サイズの DebugCamera で描画する
		CameraManager::SetTypeStatic(CameraType::Debug);
		CameraManager::SetViewportSizeStatic(ViewportType::VIEWPORT_DEBUG, {256.0f, 256.0f});
		previewContext_->GetCameraMgr()->SetAspectRatio(1.0f, 1.0f);
		// モデル全体が見える位置へ DebugCamera を配置する
		// 対象全体が見えるように DebugCamera の注視点・距離・角度を設定する
		if(auto* debugCamera = CameraManager::GetDebug()) {
			DebugCamera::State state = debugCamera->CaptureState();
			state.target			 = center;
			state.distance			 = radius * 2.8f;
			state.orbitAngle		 = {0.65f, 0.35f};
			debugCamera->ApplyState(state);
			debugCamera->AlwaysUpdate(0.0f);
		}
		previewContext_->GetCameraMgr()->TransferToGPU();
		previewContext_->GetLightLibrary()->CyncGpu();

		renderTarget->Clear(cmdList);
		renderTarget->SetRenderTarget(cmdList);

		// モデルを原点に配置し、描画対象として登録する
		WorldTransform transform = MakePreviewTransform();
		modelRenderer_->BeginFrame();
		modelRenderer_->RegisterStatic(model.get(), transform, BillboardMode::None, nullptr);

		if(auto* camera = dynamic_cast<Camera3d*>(CameraManager::GetActive())) {
			// previewはディザ抜きを行わない
			camera->SetCameraDitherEnabled(false);
			modelRenderer_->PreCullAndBatch(camera, false);
		} else {
			modelRenderer_->BuildAllVisibleBatches();
		}

		modelRenderer_->DrawAll(cmdList,
								GraphicsGroup::GetInstance()->GetDevice().Get(),
								renderTarget,
								pso,
								previewContext_->GetLightLibrary(),
								nullptr);

		const bool copied = CopyRenderTargetToCache(cmdList, renderTarget, entry);
		// GPU コマンドが完了するまで、描画に使った Model を延命する
		retainedFrameModels_.push_back(std::move(model));
		if(previous) previous->MakeCurrent();
		return copied;
	}

	///////////////////////////////////////////////////////////////////////////////
	// プレビュー描画に必要な SceneContext と ModelRenderer を準備する
	// まだ作成されていない場合のみ初期化する
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::EnsurePreviewContext() {
		// 初回のみプレビュー専用 SceneContext を生成する
		if(!previewContext_) {
			previewContext_ = std::make_unique<SceneContext>();
			previewContext_->Initialize(true);
			// プレビュー用カメラはユーザー入力で動かないようにする
			if(auto* debugCamera = CameraManager::GetDebug()) {
				debugCamera->SetInputEnabled(false);
			}
		}
		// 初回のみプレビュー用 ModelRenderer を生成する
		if(!modelRenderer_) {
			modelRenderer_ = std::make_unique<ModelRenderer>();
		}
		return previewContext_ && modelRenderer_;
	}

	///////////////////////////////////////////////////////////////////////////////
	// AssetRecord からプレビュー用の SceneObject を生成する
	// Model は BaseGameObject として生成し、Prefab は Serializer から読み込む
	///////////////////////////////////////////////////////////////////////////////
	std::vector<std::shared_ptr<SceneObject>> AssetPreviewManager::CreatePreviewObjects(const AssetRecord& record) {
		// Model アセットは、モデルを持つ BaseGameObject として一時生成する
		if(record.type == AssetType::Model) {
			auto* assetManager = AssetManager::GetInstance();
			// ModelManager 経由でモデルをロードし、ロードタスクを進める
			if(assetManager && assetManager->GetModelManager()) {
				const std::string modelName = record.sourcePath.filename().string();
				assetManager->GetModelManager()->LoadModel(modelName);
				assetManager->GetModelManager()->ProcessLoadingTasks();

				auto object = std::make_shared<BaseGameObject>(modelName, record.sourcePath.stem().string());
				object->Initialize();
				object->AlwaysUpdate(0.0f);
				return {object};
			}
			return {};
		}

		// Prefab アセットは Serializer から SceneObject 群を復元する
		if(record.type == AssetType::Prefab) {
			PrefabSerializer::LoadOptions options;
			// プレビュー用に生成するため、元の GUID は保持しない
			options.preserveGuids	 = false;
			options.prefabAssetGuid	 = record.guid;
			// 未知の型が含まれていても、プレビュー生成自体は継続できるようにする
			options.skipUnknownTypes = true;
			return PrefabSerializer::Load(record.sourcePath.string(),
										  options);
		}

		return {};
	}

	///////////////////////////////////////////////////////////////////////////////
	// SceneObject をプレビュー描画用の ModelRenderer に登録する
	// 静的モデル・アニメーションモデル・イベントオブジェクトを種別ごとに処理する
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::RegisterPreviewObject(SceneObject* object) {
		if(!object) return;

		// BaseGameObject の場合は、モデル種別に応じて登録先を切り替える
		if(auto* go = dynamic_cast<BaseGameObject*>(object)) {
			switch(go->GetModelType()) {
			case ObjectModelType::ModelType_Static:
				if(auto* model = go->GetStaticModel()) {
					modelRenderer_->RegisterStatic(model, go->GetRenderWorldTransform(), go->GetBillboardMode(), go);
				}
				break;
			case ObjectModelType::ModelType_Animation:
				if(auto* model = go->AnimationModel()) {
					modelRenderer_->RegisterSkinned(model, go->GetRenderWorldTransform(), go);
				}
				break;
			default:
				break;
			}
			return;
		}

		// EventObject がモデルを持つ場合も、静的モデルとしてプレビューに登録する
		if(auto* eventObject = dynamic_cast<BaseEventObject*>(object)) {
			if(auto* model = eventObject->GetModel()) {
				modelRenderer_->RegisterStatic(model, eventObject->GetWorldTransform(), BillboardMode::None, eventObject);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// プレビュー対象全体が画面に収まるようにカメラを設定する
	// 各オブジェクトの AABB を結合し、中心位置と距離を決定する
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::ConfigureCamera(const std::vector<std::shared_ptr<SceneObject>>& roots) {
		// すべてのプレビュー対象を内包する AABB を求める
		AABB bounds{};
		bool hasBounds = false;

		// ルートオブジェクトから子階層まで走査し、各オブジェクトの AABB を結合する
		for(const auto& root : roots) {
			std::vector<SceneObject*> flattened;
			CollectObjectsRecursive(root.get(), flattened);
			for(SceneObject* object : flattened) {
				if(auto* go = dynamic_cast<BaseGameObject*>(object)) {
					const AABB objectBounds = go->GetWorldAABB();
					bounds					= hasBounds ? MergeAabb(bounds, objectBounds) : objectBounds;
					hasBounds				= true;
				}
			}
		}

		// AABB が取得できない場合のデフォルト表示位置と距離
		Vector3 center{0.0f, 0.5f, 0.0f};
		float	radius = 1.5f;
		// AABB が取得できた場合は、その中心と大きさからカメラ距離を決める
		if(hasBounds) {
			center				  = (bounds.min_ + bounds.max_) * 0.5f;
			const Vector3 extents = (bounds.max_ - bounds.min_) * 0.5f;
			radius				  = (std::max)(0.75f, extents.Length());
		}

		CameraManager::SetTypeStatic(CameraType::Debug);
		CameraManager::SetViewportSizeStatic(ViewportType::VIEWPORT_DEBUG, {256.0f, 256.0f});
		previewContext_->GetCameraMgr()->SetAspectRatio(1.0f, 1.0f);

		if(auto* debugCamera = CameraManager::GetDebug()) {
			DebugCamera::State state = debugCamera->CaptureState();
			state.target			 = center;
			state.distance			 = radius * 2.8f;
			state.orbitAngle		 = {0.65f, 0.35f};
			debugCamera->ApplyState(state);
			debugCamera->AlwaysUpdate(0.0f);
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	// RenderTarget の内容をアセットごとのキャッシュテクスチャへコピーする
	// コピー後は SRV として参照できる状態にして ImGui 表示用 TextureID を保存する
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::CopyRenderTargetToCache(ID3D12GraphicsCommandList* cmdList,
													  IRenderTarget*			 renderTarget,
													  Entry&					 entry) {
		// コピー元となる RenderTarget のリソースを取得する
		auto* source = renderTarget->GetResource();
		if(!source || !source->Get()) return false;

		// キャッシュテクスチャ作成に使用する D3D12 Device を取得する
		auto* device = GraphicsGroup::GetInstance()->GetDevice().Get();
		if(!device) return false;

		// RenderTarget と同じサイズ・フォーマットでキャッシュ用リソースを作成する
		const auto desc = source->Get()->GetDesc();
		RetireEntryResources(entry);
		entry.cacheResource = std::make_unique<DxGpuResource>();
		entry.cacheResource->InitializeAsRenderTarget(device,
													  static_cast<uint32_t>(desc.Width),
													  desc.Height,
													  desc.Format,
													  L"AssetPreviewCache");
		entry.cacheResource->CreateSRV(device);

		// RenderTarget からキャッシュリソースへコピーできるようにステートを変更する
		renderTarget->TransitionTo(cmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		entry.cacheResource->Transition(cmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		// 描画結果をアセット専用のキャッシュテクスチャへコピーする
		cmdList->CopyResource(entry.cacheResource->Get(), source->Get());
		// コピー後は ImGui / シェーダから参照できるよう SRV 用ステートへ戻す
		entry.cacheResource->Transition(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		renderTarget->TransitionTo(cmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		// ImGui で表示できるよう、SRV の GPU ハンドルを TextureID として保持する
		entry.texture = (ImTextureID)entry.cacheResource->GetSRVGpuHandle().ptr;
		return entry.texture != nullptr;
	}

	///////////////////////////////////////////////////////////////////////////////
	// Entry が保持している GPU リソースを退避する
	// GPU がまだ使用している可能性があるため、即時破棄せずフレーム終端まで保持する
	///////////////////////////////////////////////////////////////////////////////
	void AssetPreviewManager::RetireEntryResources(Entry& entry) {
		// キャッシュリソースが存在する場合は、即時破棄せず退避リストへ移す
		if(entry.cacheResource) {
			retiredFrameResources_.push_back(std::move(entry.cacheResource));
		}
		// 表示用 TextureID は無効化する
		entry.texture = nullptr;
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセット本体の隣にあるプレビュー画像を読み込む
	// .preview.png などのサイドカー画像があれば、それをプレビューとして使用する
	///////////////////////////////////////////////////////////////////////////////
	bool AssetPreviewManager::TryLoadSidecarPreview(const AssetRecord& record, Entry& entry) {
		// アセット本体の隣にあるプレビュー画像を探す
		auto previewPath = FindSidecarPreviewPath(record);
		if(!previewPath.has_value()) return false;

		auto* db = AssetDatabase::GetInstance();
		auto* tm = AssetManager::GetInstance()->GetTextureManager();
		if(!db || !tm) return false;

		std::error_code ec;
		// DB ルートからの相対パスにして、エンジンのテクスチャ読み込み形式に合わせる
		const auto		rel = std::filesystem::relative(*previewPath, db->GetRoot(), ec);
		if(ec) return false;

		// サイドカー画像を通常のテクスチャとして読み込む
		const auto handle = tm->LoadTexture(rel.generic_string());
		if(handle.ptr == 0) return false;

		RetireEntryResources(entry);
		entry.texture = (ImTextureID)handle.ptr;
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////
	// アセット本体の近くにあるサイドカープレビュー画像を探す
	// 対応する命名規則のファイルが存在すれば、そのパスを返す
	///////////////////////////////////////////////////////////////////////////////
	std::optional<std::filesystem::path> AssetPreviewManager::FindSidecarPreviewPath(const AssetRecord& record) const {
		// 対応するサイドカー画像の命名規則
		const std::array<const char*, 4> suffixes = {
			".preview.dds",
			".preview.png",
			"_preview.dds",
			"_preview.png",
		};

		// 各命名規則に沿って候補ファイルを作り、存在確認する
		for(const char* suffix : suffixes) {
			std::filesystem::path path = record.sourcePath;
			// '_preview' 系はファイル名を置き換え、'.preview' 系は拡張子の後ろに追加する
			if(suffix[0] == '_') {
				path.replace_filename(record.sourcePath.stem().string() + suffix);
			} else {
				path += suffix;
			}

			std::error_code ec;
			if(std::filesystem::exists(path, ec) && !ec) return path;
		}

		// 対応するプレビュー画像が見つからなかった
		return std::nullopt;
	}

} // namespace CalyxEngine
