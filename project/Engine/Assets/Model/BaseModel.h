#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

/* engine */
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <engine/graphics/Material.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <Engine/Objects/Transform/Transform.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>
#include <Engine/Graphics/Shadow/Raytracing/RaytracingMesh.h>

/*data*/
#include <Data/Engine/Configs/Scene/Objects/Model/BaseModelConfig.h>

/* math */
#include <Engine/Foundation/Math/Vector4.h>

/* c++ */
#include <d3d12.h>
#include <string>
#include <optional>
#include <memory>

namespace CalyxEngine {
	class MaterialAsset;
}

/*-----------------------------------------------------------------------------------------
 * BaseModel
 * - モデル基底クラス
 * - メッシュデータ・マテリアル・テクスチャの管理と描画の共通処理を提供
 *---------------------------------------------------------------------------------------*/
class BaseModel {
public:
	//===================================================================*/
	//			public methods
	//===================================================================*/
	virtual ~BaseModel() = default;

	virtual void Initialize() = 0;
	virtual void Update(float deltaTime);
	virtual void OnModelLoaded();
	void UpdateTexture(float deltaTime);
	virtual void Map() = 0;
	virtual void ShowImGuiInterface();
	virtual void Draw(const WorldTransform& transform);
	static void BeginUploadFrame();

	//--------- config -----------------------------------------------------
	void ApplyConfig(const BaseModelConfig& config);
	BaseModelConfig ExtractConfig() const;
	void ShowImGui(BaseModelConfig& config);
	bool LoadTextureByGuid(const Guid& g);
	void SetTextureGuid(const Guid& g);
	void SetMaterialGuid(const Guid& g);

	//--------- accessor -----------------------------------------------------
	BlendMode GetBlendMode() const { return blendMode_; }
	void SetBlendMode(BlendMode mode) { blendMode_ = mode; }
	ModelData* GetModelData()const;
	const CalyxEngine::Vector4& GetColor() const;
	void SetColor(const CalyxEngine::Vector4& color);
	void SetIsDrawEnable(bool drawEnable) { isDrawEnable_ = drawEnable; }
	bool GetIsDrawEnable()const { return isDrawEnable_; }
	void SetTex(const std::string& name);
	void SetLightingMode(LightingMode mode);
	LightingMode GetLightingMode() const;
	void TransferMaterial();

	// 参照用（TLAS インスタンス登録で使う）
	D3D12_GPU_VIRTUAL_ADDRESS GetBLAS() const { return rayMesh_.GetBLAS(); }
	bool HasBLAS() const { return blasBuilt_ && rayMesh_.GetBLAS() != 0; }

	//--------- render用（レンダラーから呼ぶ軽量API） -----------------------------
	void EnsureInstanceCapacity(ID3D12Device* device, UINT needCount);
	void UploadInstanceMatrices(const std::vector<WorldTransform>& tf);

	virtual void EnsureRaytracingBLAS(ID3D12Device5* device5, ID3D12GraphicsCommandList4* cmdList4);

	// レンダラーが使うハンドル
	D3D12_GPU_DESCRIPTOR_HANDLE GetInstanceSrv()const;  //< VS:t0 (gTransMat)
	D3D12_GPU_DESCRIPTOR_HANDLE GetTexSrv()const;       //< PS:t0 (gTexture)
	D3D12_GPU_DESCRIPTOR_HANDLE GetTexSrv(size_t materialIndex)const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetMaterialGraphTextureSrvTable(size_t materialIndex) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetEnvMapSrv()const;    //< PS:t1 (gEnvironmentMap)
	const Material& GetMaterialForBatch() const { return currentMaterial_; }
	std::shared_ptr<CalyxEngine::MaterialAsset> GetMaterialAsset() const;
	const Guid& GetMaterialGuid() const { return materialGuid_; }
	const Guid& GetTextureGuid() const { return textureGuid_; }
	bool UsesRuntimeMaterialGraph() const;

	virtual void BindVertexIndexBuffers(ID3D12GraphicsCommandList* cmdList)const;
	void BindMaterialCB(ID3D12GraphicsCommandList* cmdList)const;

	// -------- billboard (VS:t1) をモデル側で保持  --------------
	void EnsureBillboardCapacity(ID3D12Device* device, UINT needCount);
	void UploadBillboardParams(const std::vector<GpuBillboardParams>&params);
	D3D12_GPU_DESCRIPTOR_HANDLE GetBillboardSrv() const;

protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	DxConstantBuffer<Material> materialBuffer_;
	Material currentMaterial_{};

	std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> handle_{};
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> materialTextureHandles_;
	mutable std::vector<DescriptorHandle> materialGraphTextureTables_;

	std::string fileName_;
	std::string textureName_ = "textures/white1x1.dds"; // デフォルトのテクスチャ名
	ModelData* modelData_;
	Guid materialGuid_;
	std::optional<CalyxEngine::Vector4> colorOverride_;
public:
	BlendMode blendMode_ = BlendMode::NORMAL;
	Transform2D  uvTransform{ {1.0f, 1.0f},
							  0.0f,
							  {0.0f, 0.0f} };

protected:
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandles_; // テクスチャハンドルリスト
	float animationSpeed_ = 0.1f; // アニメーションの速度 (秒/フレーム)
	float elapsedTime_ = 0.0f;    // 経過時間
	size_t currentFrameIndex_ = 0; // 現在のフレームインデックス

protected:
	Guid textureGuid_;
	static const std::string directoryPath_;
	bool isDrawEnable_ = true;

	virtual void CreateMaterialBuffer() = 0;
	virtual void MaterialBufferMap() = 0;

protected:
	// -------- インスタンス行列（VS:t0） -----------------------------------------
	DxStructuredBuffer<TransformationMatrix> instanceBuffer_;
	bool instanceBufferCreated_ = false;
	UINT instanceBufferCapacity_ = 0;
	std::vector<std::unique_ptr<DxStructuredBuffer<TransformationMatrix>>> instanceUploadBuffers_;
	DxStructuredBuffer<TransformationMatrix>* currentInstanceBuffer_ = nullptr;
	size_t instanceUploadCursor_ = 0;

	// -------- ビルボード（VS:t1）フレームリング -------------------------------
	DxStructuredBuffer<GpuBillboardParams> billboardBuffer_;
	UINT billboardCapacity_ = 0;
	std::vector<std::unique_ptr<DxStructuredBuffer<GpuBillboardParams>>> billboardUploadBuffers_;
	DxStructuredBuffer<GpuBillboardParams>* currentBillboardBuffer_ = nullptr;
	size_t billboardUploadCursor_ = 0;
	uint64_t uploadFrameGenerationSeen_ = 0;


	CalyxEngine::RaytracingMesh rayMesh_;
	bool blasBuilt_ = false;
};
