#include "RaytracingMesh.h"

#include "Engine/Assets/Model/ModelData.h"

#include <cassert>

namespace CalyxGraphics {

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
				assert(false); // デバッグ時に失敗を検出
			}
			return res; // 空の ComPtr を返す（呼び出し側でチェック）
		}
	} // namespace

	/////////////////////////////////////////////////////////////////////////////////
	//	BLAS構築
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingMesh::BuildBLAS(
		ID3D12Device5*				device,
		ID3D12GraphicsCommandList4* cmd,
		const ModelData&			model) {
		assert(device && cmd);

		const auto& vb = model.meshResource.VertexBuffer();
		const auto& ib = model.meshResource.IndexBuffer();

		// -----------------------------
		// Geometry Desc
		// -----------------------------
		D3D12_RAYTRACING_GEOMETRY_DESC geom{};
		geom.Type  = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geom.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		geom.Triangles.VertexBuffer.StartAddress =
			vb.GetResource()->GetGPUVirtualAddress();
		geom.Triangles.VertexBuffer.StrideInBytes =
			sizeof(VertexPosUvN);
		geom.Triangles.VertexCount =
			static_cast<UINT>(model.meshResource.Vertices().size());
		geom.Triangles.VertexFormat =
			DXGI_FORMAT_R32G32B32_FLOAT; // position only

		geom.Triangles.IndexBuffer =
			ib.GetResource()->GetGPUVirtualAddress();
		geom.Triangles.IndexCount =
			static_cast<UINT>(model.meshResource.Indices().size());
		geom.Triangles.IndexFormat =
			DXGI_FORMAT_R32_UINT;

		geom.Triangles.Transform3x4 = 0;

		// -----------------------------
		// Build Inputs
		// -----------------------------
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.Type =
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		inputs.DescsLayout	  = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.NumDescs		  = 1;
		inputs.pGeometryDescs = &geom;
		inputs.Flags =
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		// -----------------------------
		// Prebuild Info
		// -----------------------------
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
		device->GetRaytracingAccelerationStructurePrebuildInfo(
			&inputs, &prebuild);
		assert(prebuild.ResultDataMaxSizeInBytes > 0);
		assert(prebuild.ScratchDataSizeInBytes > 0);

		// -----------------------------
		// Resource Create
		// -----------------------------
		blas_ = CreateRaytracingBuffer(
			device,
			prebuild.ResultDataMaxSizeInBytes,
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		scratch_ = CreateRaytracingBuffer(
			device,
			prebuild.ScratchDataSizeInBytes,
			D3D12_RESOURCE_STATE_COMMON);

		// Transition Barriers
		// BLAS: Created in RTAS (No barrier needed if supported, otherwise implicit promotion might trigger?)
		// Actually, if we get "Ignoring InitialState", it is COMMON.
		// EQUIVALENT: Try RTAS initial. If it works, great.

		D3D12_RESOURCE_BARRIER barriers[1] = {};

		// Scratch: COMMON -> UAV
		barriers[0].Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barriers[0].Transition.pResource   = scratch_.Get();
		barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
		barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		cmd->ResourceBarrier(1, barriers);

		// -----------------------------
		// Build BLAS
		// -----------------------------
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};
		build.Inputs = inputs;
		build.ScratchAccelerationStructureData =
			scratch_->GetGPUVirtualAddress();
		build.DestAccelerationStructureData =
			blas_->GetGPUVirtualAddress();

		cmd->BuildRaytracingAccelerationStructure(&build, 0, nullptr);

		// UAV barrier
		D3D12_RESOURCE_BARRIER uav{};
		uav.Type		  = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uav.UAV.pResource = blas_.Get();
		cmd->ResourceBarrier(1, &uav);
	}

	D3D12_GPU_VIRTUAL_ADDRESS RaytracingMesh::GetBLAS() const {
		// BLASがなければ0を返す
		return blas_
				   ? blas_->GetGPUVirtualAddress()
				   : 0;
	}

} // namespace CalyxGraphics