#pragma once

/* math */
#include <Engine/Foundation/Math/Matrix4x4.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Math/Vector4.h>

/* engine */
#include <Engine/Assets/Animation/AnimationStruct.h>
#include <Engine/Assets/Model/ModelData.h>
#include <Engine/Renderer/Mesh/VertexData.h>
#include "engine/graphics/Material.h"
#include <Engine/Objects/3D/Geometory/AABB.h>

/* c++ */
#include <list>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <DirectXTex.h>
#include <stdint.h>
#include <string>
#include <random>
#include <memory>
#include<assimp/scene.h>

class  Model;

Microsoft::WRL::ComPtr<ID3D12Resource>CreateBufferResource(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t sizeInBytes);

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

Animation LoadAnimationFile(const std::string& directoryPath, const std::string& filename);

DirectX::ScratchImage LoadTextureImage(const std::string& filePath);

bool IsCollision(const AABB& aabb, const CalyxMath::Vector3& point);

// skeletonの作成
Skeleton CreateSkeleton(const Node& rootNode);

// jointの作成
int32_t CreateJoint(const Node& node,
					const std::optional<int32_t>& parent,
					std::vector<Joint>& joints);

Node ConvertAssimpNode(const aiNode* assimpNode);

// skinClusterの作成
SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
							  const Skeleton& skeleton,const ModelData& modelData);


//ビルボードの関数
CalyxMath::Matrix4x4 MakeYAxisBillboard(const CalyxMath::Matrix4x4& cameraMatrix);

CalyxMath::Matrix4x4 MakeXAxisBillboard(const CalyxMath::Matrix4x4& cameraMatrix);
CalyxMath::Matrix4x4 MakeZAxisBillboard(const CalyxMath::Matrix4x4& cameraMatrix);