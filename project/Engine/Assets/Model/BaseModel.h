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

/*data*/
#include <Data/Engine/Configs/Scene/Objects/Model/BaseModelConfig.h>

/* math */
#include <Engine/Foundation/Math/Vector4.h>

/* c++ */
#include <d3d12.h>
#include <memory>
#include <string>
#include <wrl.h>

/* ========================================================================
/*		model
/* ===================================================================== */
class BaseModel{
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

	void DrawInstanced(const std::vector<WorldTransform>& transforms, ID3D12GraphicsCommandList* cmdList);

	//--------- config -----------------------------------------------------
	void ApplyConfig(const BaseModelConfig& config);
	BaseModelConfig ExtractConfig() const;
	void ShowImGui(BaseModelConfig& config);

	//--------- accessor -----------------------------------------------------
	BlendMode GetBlendMode() const  { return blendMode_; }
	const std::optional<ModelData>& GetModelData()const;
	const Vector4& GetColor() const { return materialData_.color; }
	void SetColor(const Vector4& color) { materialData_.color = color; }
	void SetIsDrawEnable(bool drawEnable){ isDrawEnable_ = drawEnable; }
	bool GetIsDrawEnable()const{ return isDrawEnable_; }

protected:
	//===================================================================*/
	//			protected methods
	//===================================================================*/
	DxConstantBuffer<Material> materialBuffer_;

	std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> handle_ {};

	std::string fileName_;
	std::string textureName_ = "textures/white1x1.png"; // デフォルトのテクスチャ名
	std::optional<ModelData> modelData_;
	Material materialData_;
public:
	BlendMode blendMode_ = BlendMode::NORMAL;
	Transform2D  uvTransform {{1.0f, 1.0f},
							 0.0f,
							 {0.0f, 0.0f}};


protected:
	std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> textureHandles_; // テクスチャハンドルリスト
	float animationSpeed_ = 0.1f; // アニメーションの速度 (秒/フレーム)
	float elapsedTime_ = 0.0f; // 経過時間
	size_t currentFrameIndex_ = 0; // 現在のフレームインデックス

protected:
	static const std::string directoryPath_;
	bool isDrawEnable_ = true;

	virtual void CreateMaterialBuffer() = 0;
	virtual void MaterialBufferMap() = 0;

protected:
	DxStructuredBuffer<TransformationMatrix> instanceBuffer_;
	bool instanceBufferCreated_ = false;
	UINT instanceBufferCapacity_ = 0;
};
