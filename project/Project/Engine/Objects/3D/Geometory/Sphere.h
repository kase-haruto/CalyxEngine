//#pragma once
//
//#include <Engine/Assets/Model/ModelData.h>
//#include "engine/graphics/Material.h"
//#include <Engine/Objects/Transform/TransformationMatrix.h>
//#include <Engine/Objects/Transform/Transform.h>
//
///* math */
//#include <Engine/Foundation/Math/Vector4.h>
//
///* c++ */
//#include <string>
//#include <d3d12.h>
//#include <wrl.h>
//
//class DirectXCommon;
//
///// <summary>
///// モデル
///// </summary>
//class Sphere{
//public:
//	Sphere();
//	~Sphere();
//
//	/// <summary>
//	/// 初期化
//	/// </summary>
//	void Initialize();
//	/// <summary>
//	/// 更新
//	/// </summary>
//	void Update();
//	/// <summary>
//	/// 描画
//	/// </summary>
//	void Draw();
//	/// <summary>
//	/// リソースの生成
//	/// </summary>
//	void CreateBuffer();
//	/// <summary>
//	/// マップする
//	/// </summary>
//	void Map();
//
//	void UpdateImGui(std::string lavel);
//
//	void MatrixInitialize();
//
//private:
//	/// <summary>
//	/// リソースの生成
//	/// </summary>
//	void CreateVertexBuffer();
//	void CreateMaterialBuffer();
//	void CreateMatrixBuffer();
//	/// <summary>
//	/// マップする
//	/// </summary>
//	void VertexBufferMap();
//	void MaterialBufferMap();
//	void MatrixBufferMap();
//
//
//private:
//	///=============================================================
//	///	directX関連
//	///=============================================================
//	Microsoft::WRL::ComPtr<ID3D12Device> device_;
//	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>commandList_;
//	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature_;
//	Microsoft::WRL::ComPtr<ID3D12PipelineState>pipelineState_;
//
//	///=============================================================
//	///	Resources
//	///=============================================================
//	D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
//	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
//	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
//	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
//
//	D3D12_GPU_DESCRIPTOR_HANDLE handle;
//
//	int currentLightingMode =0;
//
//	Vector4 RGBa;
//	
//	Material* materialData;
//	TransformationMatrix* matrixData;
//
//public:
//	Transform transform;
//};
//
