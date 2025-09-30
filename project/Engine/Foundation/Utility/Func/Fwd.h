#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct Matrix4x4;
struct Vector2;
struct Vector3;
struct Vector4;
struct Quaternion;
class AABB;


struct ModelData;
struct MaterialData;
struct Animation;


struct Node;
struct Joint;
struct Skeleton;
struct SkinCluster;


namespace Microsoft { namespace WRL { template<class T> class ComPtr; } }
struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12DescriptorHeap;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;


namespace DirectX { class ScratchImage; }
struct aiNode;