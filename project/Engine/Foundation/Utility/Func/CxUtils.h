#pragma once
#include "Fwd.h"

#include <d3d12.h>

namespace CalyxMath {
	[[nodiscard]] Matrix4x4 MakeTranslateMatrix(const Vector3&) noexcept;
	[[nodiscard]] Matrix4x4 MakeScaleMatrix(const Vector3&) noexcept;
	[[nodiscard]] Matrix4x4 MakeRotateXMatrix(float) noexcept;
	[[nodiscard]] Matrix4x4 MakeRotateYMatrix(float) noexcept;
	[[nodiscard]] Matrix4x4 MakeRotateZMatrix(float) noexcept;
	[[nodiscard]] Matrix4x4 MakeAffineMatrix(const Vector3&, const Vector3&, const Vector3&) noexcept;
	[[nodiscard]] Matrix4x4 MakeAffineMatrix(const Vector3&, const Quaternion&, const Vector3&) noexcept;
	[[nodiscard]] Matrix4x4 MakeOrthographicMatrix(float l, float t, float r, float b, float n, float f) noexcept;
	[[nodiscard]] Matrix4x4 MakeOrthographicMatrixLH(
	float l, float r,
float b, float t,
float nearClip,
float farClip);

	[[nodiscard]] Vector3 TransformNormal(const Vector3&, const Matrix4x4&) noexcept;
	[[nodiscard]] Vector4 MultiplyMatrixVector(const Matrix4x4&, const Vector4&) noexcept;

	[[nodiscard]] float Lerp(float a, float b, float t) noexcept;
	[[nodiscard]] float LerpShortAngle(float a, float b, float t) noexcept;
	[[nodiscard]] float ToRadians(float v) noexcept;

	[[nodiscard]] Vector2 WorldToScreen(const Vector3& worldPos);
	[[nodiscard]] Vector3 ScreenToWorld(const Vector2& screenPos, float depthZ);
} // namespace CalyxMath

namespace Cx {

	namespace Dx {
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
																	size_t										sizeInBytes);

		[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
			ID3D12DescriptorHeap* heap, uint32_t descriptorSize, uint32_t index) noexcept;

		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
			ID3D12DescriptorHeap* heap, uint32_t descriptorSize, uint32_t index) noexcept;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr<ID3D12Device> device,
																		  D3D12_DESCRIPTOR_HEAP_TYPE		   heapType,
																		  UINT								   numDescriptors,
																		  bool								   shaderVisible);

		void DrawTextureToRenderTarget(ID3D12GraphicsCommandList*  cmd,
									   D3D12_GPU_DESCRIPTOR_HANDLE inputSRV,
									   class IRenderTarget*		   output,
									   ID3D12PipelineState*		   pso,
									   ID3D12RootSignature*		   rootSig);
	} // namespace Dx

	namespace IO {
		[[nodiscard]] ModelData				LoadObjFile(const std::string& directory, const std::string& filename);
		[[nodiscard]] MaterialData			LoadMaterialTemplateFile(const std::string& directory, const std::string& filename);
		[[nodiscard]] Animation				LoadAnimationFile(const std::string& directory, const std::string& filename);
		[[nodiscard]] DirectX::ScratchImage LoadTextureImage(const std::string& filePath);
	} // namespace IO

	namespace Anim {
		[[nodiscard]] Node		  ConvertAssimpNode(const aiNode* assimpNode);
		[[nodiscard]] Skeleton	  CreateSkeleton(const Node& rootNode);
		int32_t					  CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);
		[[nodiscard]] SkinCluster CreateSkinCluster(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
													const Skeleton& skeleton, const ModelData& modelData);
	} // namespace Anim

	namespace Collision {
		[[nodiscard]] bool IsCollision(const AABB& aabb, const Vector3& point) noexcept;
	}

	namespace Billboard {
		[[nodiscard]] Matrix4x4 MakeYAxisBillboard(const Matrix4x4& cameraMatrix) noexcept;
		[[nodiscard]] Matrix4x4 MakeXAxisBillboard(const Matrix4x4& cameraMatrix) noexcept;
		[[nodiscard]] Matrix4x4 MakeZAxisBillboard(const Matrix4x4& cameraMatrix) noexcept;
	} // namespace Billboard
} // namespace Cx
