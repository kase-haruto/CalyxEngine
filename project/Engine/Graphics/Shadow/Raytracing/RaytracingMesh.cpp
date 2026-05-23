#include "RaytracingMesh.h"

#include "Engine/Assets/Model/ModelData.h"

#include <algorithm>
#include <Engine/Foundation/Debug/CxAssert.h>
#include <cstring>

namespace CalyxEngine {

	namespace {
		/////////////////////////////////////////////////////////////////////////////////////
		//	DefaultBuffer作成
		/////////////////////////////////////////////////////////////////////////////////////
		/////////////////////////////////////////////////////////////////////////////////////
		//	RaytracingBuffer作成 (Flags強制)
		/////////////////////////////////////////////////////////////////////////////////////
		Microsoft::WRL::ComPtr<ID3D12Resource> CreateRaytracingBuffer(
			ID3D12Device*		  device,
			size_t				  bytes,
			D3D12_RESOURCE_STATES initialState) {
			Microsoft::WRL::ComPtr<ID3D12Resource> res;

			D3D12_HEAP_PROPERTIES heap{};
			heap.Type = D3D12_HEAP_TYPE_DEFAULT;

			D3D12_RESOURCE_DESC desc{};
			desc.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Width			  = bytes;
			desc.Height			  = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels		  = 1;
			desc.SampleDesc.Count = 1;
			desc.Layout			  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags			  = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // Force flags

			HRESULT hr = device->CreateCommittedResource(
				&heap,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				initialState,
				nullptr,
				IID_PPV_ARGS(&res));
			if(FAILED(hr)) {
				CX_CHECK(false, "Assertion failed"); // デバッグ時に失敗を検出
			}
			return res; // 空の ComPtr を返す（呼び出し側でチェック）
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(
			ID3D12Device* device,
			const void* data,
			size_t bytes) {
			Microsoft::WRL::ComPtr<ID3D12Resource> res;

			D3D12_HEAP_PROPERTIES heap{};
			heap.Type = D3D12_HEAP_TYPE_UPLOAD;

			D3D12_RESOURCE_DESC desc{};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Width = bytes;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.SampleDesc.Count = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			HRESULT hr = device->CreateCommittedResource(
				&heap,
				D3D12_HEAP_FLAG_NONE,
				&desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&res));
			CX_CHECK(SUCCEEDED(hr), "Assertion failed");

			void* mapped = nullptr;
			hr = res->Map(0, nullptr, &mapped);
			CX_CHECK(SUCCEEDED(hr) && mapped, "Assertion failed");
			std::memcpy(mapped, data, bytes);
			res->Unmap(0, nullptr);

			return res;
		}

		void BuildBLASInternal(
			ID3D12Device5* device,
			ID3D12GraphicsCommandList4* cmd,
			D3D12_GPU_VIRTUAL_ADDRESS vertexAddress,
			UINT vertexCount,
			UINT vertexStride,
			D3D12_GPU_VIRTUAL_ADDRESS indexAddress,
			UINT indexCount,
			Microsoft::WRL::ComPtr<ID3D12Resource>& blas,
			Microsoft::WRL::ComPtr<ID3D12Resource>& scratch,
			bool update) {
			CX_CHECK(device && cmd, "Assertion failed");

			D3D12_RAYTRACING_GEOMETRY_DESC geom{};
			geom.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
			geom.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
			geom.Triangles.VertexBuffer.StartAddress = vertexAddress;
			geom.Triangles.VertexBuffer.StrideInBytes = vertexStride;
			geom.Triangles.VertexCount = vertexCount;
			geom.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
			geom.Triangles.IndexBuffer = indexAddress;
			geom.Triangles.IndexCount = indexCount;
			geom.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
			geom.Triangles.Transform3x4 = 0;

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
			inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
			inputs.NumDescs = 1;
			inputs.pGeometryDescs = &geom;
			inputs.Flags =
				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE |
				D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
			if(update) {
				inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
			}

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
			device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuild);
			CX_CHECK(prebuild.ResultDataMaxSizeInBytes > 0, "Assertion failed");
			CX_CHECK(prebuild.ScratchDataSizeInBytes > 0, "Assertion failed");

			bool createdScratch = false;

			if(!blas) {
				blas = CreateRaytracingBuffer(
					device,
					prebuild.ResultDataMaxSizeInBytes,
					D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
			}

			if(!scratch) {
				const size_t scratchBytes = (std::max)(
					static_cast<size_t>(prebuild.ScratchDataSizeInBytes),
					static_cast<size_t>(prebuild.UpdateScratchDataSizeInBytes));
				scratch = CreateRaytracingBuffer(
					device,
					scratchBytes,
					D3D12_RESOURCE_STATE_COMMON);
				createdScratch = true;
			}

			if(createdScratch) {
				D3D12_RESOURCE_BARRIER barrier{};
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = scratch.Get();
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
				barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				cmd->ResourceBarrier(1, &barrier);
			}

			D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};
			build.Inputs = inputs;
			build.ScratchAccelerationStructureData = scratch->GetGPUVirtualAddress();
			build.DestAccelerationStructureData = blas->GetGPUVirtualAddress();
			if(update) {
				build.SourceAccelerationStructureData = blas->GetGPUVirtualAddress();
			}
			cmd->BuildRaytracingAccelerationStructure(&build, 0, nullptr);

			D3D12_RESOURCE_BARRIER uav{};
			uav.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			uav.UAV.pResource = blas.Get();
			cmd->ResourceBarrier(1, &uav);
		}
	} // namespace

	/////////////////////////////////////////////////////////////////////////////////
	//	BLAS構築
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingMesh::BuildBLAS(
		ID3D12Device5*				device,
		ID3D12GraphicsCommandList4* cmd,
		const ModelData&			model) {
		CX_CHECK(device && cmd, "Assertion failed");

		const auto& vb = model.meshResource.VertexBuffer();
		const auto& ib = model.meshResource.IndexBuffer();
		if(blas_ && scratch_) {
			BuildBLASInternal(
				device,
				cmd,
				vb.GetResource()->GetGPUVirtualAddress(),
				static_cast<UINT>(model.meshResource.Vertices().size()),
				sizeof(VertexPosUvN),
				ib.GetResource()->GetGPUVirtualAddress(),
				static_cast<UINT>(model.meshResource.Indices().size()),
				blas_,
				scratch_,
				true);
			return;
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> newBlas;
		Microsoft::WRL::ComPtr<ID3D12Resource> newScratch;
		BuildBLASInternal(
			device,
			cmd,
			vb.GetResource()->GetGPUVirtualAddress(),
			static_cast<UINT>(model.meshResource.Vertices().size()),
			sizeof(VertexPosUvN),
			ib.GetResource()->GetGPUVirtualAddress(),
			static_cast<UINT>(model.meshResource.Indices().size()),
			newBlas,
			newScratch,
			false);

		RetireResource(blas_);
		RetireResource(scratch_);
		blas_	 = newBlas;
		scratch_ = newScratch;
	}

	void RaytracingMesh::BuildBLAS(
		ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmd,
		const std::vector<VertexPosUvN>& vertices,
		const std::vector<uint32_t>& indices) {
		CX_CHECK(device && cmd, "Assertion failed");
		if(vertices.empty() || indices.empty()) return;

		Microsoft::WRL::ComPtr<ID3D12Resource> newVertexUpload = CreateUploadBuffer(
			device,
			vertices.data(),
			sizeof(VertexPosUvN) * vertices.size());
		Microsoft::WRL::ComPtr<ID3D12Resource> newIndexUpload = CreateUploadBuffer(
			device,
			indices.data(),
			sizeof(uint32_t) * indices.size());

		Microsoft::WRL::ComPtr<ID3D12Resource> newBlas;
		Microsoft::WRL::ComPtr<ID3D12Resource> newScratch;
		BuildBLASInternal(
			device,
			cmd,
			newVertexUpload->GetGPUVirtualAddress(),
			static_cast<UINT>(vertices.size()),
			sizeof(VertexPosUvN),
			newIndexUpload->GetGPUVirtualAddress(),
			static_cast<UINT>(indices.size()),
			newBlas,
			newScratch,
			false);

		RetireResource(blas_);
		RetireResource(scratch_);
		RetireResource(vertexUpload_);
		RetireResource(indexUpload_);
		blas_		  = newBlas;
		scratch_	  = newScratch;
		vertexUpload_ = newVertexUpload;
		indexUpload_  = newIndexUpload;
	}

	void RaytracingMesh::BuildBLAS(
		ID3D12Device5* device,
		ID3D12GraphicsCommandList4* cmd,
		D3D12_GPU_VIRTUAL_ADDRESS vertexAddress,
		UINT vertexCount,
		UINT vertexStride,
		D3D12_GPU_VIRTUAL_ADDRESS indexAddress,
		UINT indexCount) {
		if(vertexAddress == 0 || indexAddress == 0 || vertexCount == 0 || indexCount == 0) return;

		if(blas_ && scratch_) {
			BuildBLASInternal(
				device,
				cmd,
				vertexAddress,
				vertexCount,
				vertexStride,
				indexAddress,
				indexCount,
				blas_,
				scratch_,
				true);
			return;
		}

		Microsoft::WRL::ComPtr<ID3D12Resource> newBlas;
		Microsoft::WRL::ComPtr<ID3D12Resource> newScratch;
		BuildBLASInternal(
			device,
			cmd,
			vertexAddress,
			vertexCount,
			vertexStride,
			indexAddress,
			indexCount,
			newBlas,
			newScratch,
			false);

		RetireResource(blas_);
		RetireResource(scratch_);
		blas_	 = newBlas;
		scratch_ = newScratch;
	}

	D3D12_GPU_VIRTUAL_ADDRESS RaytracingMesh::GetBLAS() const {
		// BLASがなければ0を返す
		return blas_
				   ? blas_->GetGPUVirtualAddress()
				   : 0;
	}

	void RaytracingMesh::RetireResource(Microsoft::WRL::ComPtr<ID3D12Resource>& resource) {
		if(!resource) return;

		retiredResources_.push_back(resource);
		resource.Reset();
	}

} // namespace CalyxEngine
