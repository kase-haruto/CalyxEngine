#pragma once

#include "Engine/Objects/3D/Actor/SceneObject.h"

#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Graphics/Shadow/Raytracing/RaytracingScene.h>
#include <Engine/Graphics/Shadow/Raytracing/RaytracingSystem.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <Engine/Objects/3D/Geometory/AABB.h>
#include <Engine/Objects/Transform/Transform.h>

#include <d3d12.h>
#include <map>
#include <unordered_map>
#include <vector>


class BaseModel;
class Camera3d;
class LightLibrary;

namespace CalyxAssets {
	class AnimationModel;
}

namespace CalyxGraphics {
	class ShadowMapSystem;
}

/*-----------------------------------------------------------------------------------------
 * ModelRenderer
 * - 3Dモデル描画管理クラス
 * - スタティック/スキンメッシュモデルのバッチング、カリング、描画制御を担当
 *---------------------------------------------------------------------------------------*/
class ModelRenderer {
public:
	struct RenderInstance {
		BaseModel*			  model;
		const WorldTransform* transform;
		SceneObject*		  owner;
	};

private:
	//===================================================================*/
	//                    private types
	//===================================================================*/
	struct InstanceStatic {
		WorldTransform tf;							  //< ワールド変換
		AABB		   worldAABB{};					  //< ワールドAABB
		bool		   dirty   = true;				  //< ダーティフラグ
		bool		   visible = false;				  //< 可視フラグ
		BillboardMode  mode	   = BillboardMode::None; //< ビルボードモード
		SceneObject*   owner   = nullptr;			  //< 所有オブジェクト
	};

	struct InstanceSkinned {
		WorldTransform tf;				  //< ワールド変換
		AABB		   worldAABB{};		  //< ワールドAABB
		bool		   dirty   = true;	  //< ダーティフラグ
		bool		   visible = false;	  //< 可視フラグ
		SceneObject*   owner   = nullptr; //< 所有オブジェクト
	};

	using PipelineKey		= PipelineService::PipelineKey;
	using PipelineKeyHasher = PipelineService::PipelineKeyHasher;

	struct StaticBatchItem {
		BaseModel*							   model = nullptr; //< モデルデータ
		std::vector<WorldTransform>			   transforms;		//< インスタンス用変換リスト
		std::vector<GpuBillboardParams>		   billboards;		//< インスタンス用ビルボードパラメータ
		DxStructuredBuffer<GpuBillboardParams> billboardSrv;	//< ビルボード用構造化バッファ
	};

	using StaticBatch  = std::vector<StaticBatchItem>;
	using SkinnedBatch = std::vector<std::pair<CalyxAssets::AnimationModel*, std::vector<WorldTransform>>>;

public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief コンストラクタ
	 */
	ModelRenderer();

	/**
	 * \brief スタティックモデルを登録
	 * \param model モデルデータ
	 * \param transform ワールド変換
	 * \param billMode ビルボードモード
	 */
	void RegisterStatic(BaseModel* model, const WorldTransform& transform, BillboardMode billMode, SceneObject* owner = nullptr);

	/**
	 * \brief スキンメッシュモデルを登録
	 * \param model アニメーションモデルデータ
	 * \param transform ワールド変換
	 */
	void RegisterSkinned(CalyxAssets::AnimationModel* model, const WorldTransform& transform, SceneObject* owner = nullptr);

	/**
	 * \brief 登録をクリア
	 */
	void Clear();

	/**
	 * \brief フレーム開始処理
	 */
	void BeginFrame();

	/**
	 * \brief カリングおよびバッチング事前処理
	 * \param camera カメラ
	 */
	void PreCullAndBatch(const class Camera3d* camera);

	/**
	 * \brief 一括描画処理
	 * \param cmdList コマンドリスト
	 * \param device デバイス
	 * \param camera カメラ
	 * \param psoService パイプラインサービス
	 * \param lightLibrary ライトライブラリ
	 * \param shadowMapSystem シャドウマップシステム
	 */
	void DrawAll(ID3D12GraphicsCommandList*		 cmdList,
				 ID3D12Device*					 device,
				 class IRenderTarget* rt,
				 class PipelineService*			 psoService,
				 class LightLibrary*			 lightLibrary,
				 CalyxGraphics::ShadowMapSystem* shadowMapSystem);

	// Picking / Outline / IDPass 用
	/**
	 * \brief 可視スタティックモデルリストを収集
	 * \param out
	 */
	void CollectVisibleStatic(std::vector<RenderInstance>& out) const;
	/**
	 * \brief 可視スキンメッシュモデルリストを収集
	 * \param out
	 */
	void CollectVisibleSkinned(std::vector<RenderInstance>& out) const;

	/**
	 * \brief 可視スタティックモデルリストを取得（シャドウ用）
	 * \return モデルデータマップ
	 */
	const std::unordered_map<BaseModel*, std::vector<WorldTransform>>&
	GetStaticVisible() const { return staticVisibleForShadow_; }

	/**
	 * \brief 可視スキンメッシュモデルリストを取得（シャドウ用）
	 * \return モデルデータマップ
	 */
	const std::unordered_map<CalyxAssets::AnimationModel*, std::vector<WorldTransform>>&
	GetSkinnedVisible() const { return skinnedVisibleForShadow_; }

	/**
	 * \brief シーンのAABBを取得
	 * \return AABB
	 */
	const AABB& GetSceneBounds() const { return sceneBounds_; }

	/**
	 * \brief シーンのAABBが存在するか
	 * \return 存在するか
	 */
	bool HasSceneBounds() const { return hasSceneBounds_; }

private:
	/**
	 * \brief スタティックモデルのダーティフラグをセット
	 * \param model モデル
	 * \param index インデックス
	 */
	void MarkStaticDirty(BaseModel* model, size_t index);
	/**
	 * \brief スキンメッシュモデルのダーティフラグをセット
	 * \param model モデル
	 * \param index インデックス
	 */
	void MarkSkinnedDirty(CalyxAssets::AnimationModel* model, size_t index);
	/**
	 * \brief スタティックモデルのバッチ構築
	 */
	void BuildStaticBatches();
	/**
	 * \brief スキンメッシュモデルのバッチ構築
	 */
	void BuildSkinnedBatches();

	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	std::unordered_map<BaseModel*, std::vector<InstanceStatic>>					   staticModels_;  //< スタティックモデル管理マップ
	std::unordered_map<CalyxAssets::AnimationModel*, std::vector<InstanceSkinned>> skinnedModels_; //< スキンメッシュモデル管理マップ

	std::map<PipelineKey, StaticBatch>	staticBatches_;	 //< スタティックモデルバッチマップ
	std::map<PipelineKey, SkinnedBatch> skinnedBatches_; //< スキンメッシュモデルバッチマップ

	std::vector<WorldTransform> tempVisibleStatic_;	 //< 一時可視スタティックリスト
	std::vector<WorldTransform> tempVisibleSkinned_; //< 一時可視スキンメッシュリスト

	AABB sceneBounds_{};		  //< シーン境界
	bool hasSceneBounds_ = false; //< シーン境界有効フラグ

	std::unordered_map<BaseModel*, std::vector<WorldTransform>>					  staticVisibleForShadow_;	//< シャドウ用可視スタティックリスト
	std::unordered_map<CalyxAssets::AnimationModel*, std::vector<WorldTransform>> skinnedVisibleForShadow_; //< シャドウ用可視スキンメッシュリスト

	// Raytracing
	std::unique_ptr<CalyxGraphics::RaytracingSystem> raytracingSystem_; //< レイトレーシングシステム
	CalyxGraphics::RaytracingScene					 raytracingScene_;	//< レイトレーシングシーン
};