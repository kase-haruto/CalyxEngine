#include "RaytracingSystem.h"

/* ========================================================================
/*	include space
/* ===================================================================== */
#include "RaytracingScene.h"

#include <cassert>

namespace CalyxGraphics {

	namespace {

		/////////////////////////////////////////////////////////////////////////////
		//  デフォルトバッファの生成
		/////////////////////////////////////////////////////////////////////////////
		Microsoft::WRL::ComPtr<ID3D12Resource>
		CreateDefaultBuffer(
			ID3D12Device*		  device,
			size_t				  bytes,
			D3D12_RESOURCE_STATES initialState,
			D3D12_RESOURCE_FLAGS  flags = D3D12_RESOURCE_FLAG_NONE) {
			Microsoft::WRL::ComPtr<ID3D12Resource> res;

			// バッファの設定
			D3D12_HEAP_PROPERTIES heap{};
			heap.Type = D3D12_HEAP_TYPE_DEFAULT;

			// リソースの設定
			D3D12_RESOURCE_DESC desc{};
			desc.Dimension		  = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Width			  = bytes;
			desc.Height			  = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels		  = 1;
			desc.SampleDesc.Count = 1;
			desc.Layout			  = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags			  = flags;

			// リソースの生成
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
	//  初期化処理
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingSystem::Initialize(ID3D12Device5* device) {
		assert(device && "RaytracingSystem::Initialize: device is null");
		device_ = device;

		// TLAS用SRVディスクリプタの確保
		tlasSrv_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
		assert(tlasSrv_.IsValid() && "RaytracingSystem::Initialize: Failed to allocate TLAS SRV descriptor");
	}

	/////////////////////////////////////////////////////////////////////////////////
	//  TLASの構築
	/////////////////////////////////////////////////////////////////////////////////
	void RaytracingSystem::BuildTLAS(
		ID3D12GraphicsCommandList4* cmd,
		const RaytracingScene&		scene) {
		assert(device_ && cmd);

		// インスタンス数の取得
		const uint32_t instanceCount = scene.GetInstanceCount();
		if(instanceCount == 0) return;

		// 入力情報の設定
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs{};
		inputs.Type =
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		inputs.DescsLayout	 = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.NumDescs		 = instanceCount;
		inputs.InstanceDescs = scene.GetInstanceDescGPUVA();
		inputs.Flags =
			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

		// 事前ビルド情報の取得
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild{};
		device_->GetRaytracingAccelerationStructurePrebuildInfo(
			&inputs, &prebuild);

		// サイズのアサート
		assert(prebuild.ResultDataMaxSizeInBytes > 0);
		assert(prebuild.ScratchDataSizeInBytes > 0);

		// TLASとスクラッチバッファの生成
		if(!tlas_ || tlasCapacity_ < instanceCount) {
			tlasCapacity_ = instanceCount;

			// バッファの作成
			tlas_ = CreateDefaultBuffer(
				device_,
				prebuild.ResultDataMaxSizeInBytes,
				D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			// バッファの作成
			scratch_ = CreateDefaultBuffer(
				device_,
				prebuild.ScratchDataSizeInBytes,
				D3D12_RESOURCE_STATE_COMMON,
				D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			// No transitions needed for TLAS (Created as RTAS)
			// Need transition for Scratch: COMMON -> UAV

			D3D12_RESOURCE_BARRIER barriers[1] = {};

			// Scratch: COMMON -> UAV
			barriers[0].Type				   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barriers[0].Transition.pResource   = scratch_.Get();
			barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
			barriers[0].Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			cmd->ResourceBarrier(1, barriers);

			// SRVの設定
			D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
			srv.ViewDimension =
				D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			srv.Shader4ComponentMapping =
				D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srv.RaytracingAccelerationStructure.Location =
				tlas_->GetGPUVirtualAddress();

			device_->CreateShaderResourceView(
				nullptr, &srv, tlasSrv_.cpu);
		}

		// TLASのビルド
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC build{};
		build.Inputs = inputs;
		build.ScratchAccelerationStructureData =
			scratch_->GetGPUVirtualAddress();
		build.DestAccelerationStructureData =
			tlas_->GetGPUVirtualAddress();

		// TLASの構築コマンド発行
		cmd->BuildRaytracingAccelerationStructure(&build, 0, nullptr);

		// UAVバリアの発行
		D3D12_RESOURCE_BARRIER uav{};
		uav.Type		  = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		uav.UAV.pResource = tlas_.Get();
		cmd->ResourceBarrier(1, &uav);
	}

	//////////////////////////////////////////////////////////////////////////////
	//  TLASの取得
	//////////////////////////////////////////////////////////////////////////////
	D3D12_GPU_DESCRIPTOR_HANDLE RaytracingSystem::GetTLASSrv() const { return tlasSrv_.gpu; }

	ID3D12Resource* RaytracingSystem::GetTLAS() const { return tlas_.Get(); }

} // namespace CalyxGraphics