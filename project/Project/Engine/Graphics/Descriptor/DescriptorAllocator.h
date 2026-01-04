#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <mutex>
#include <stack>

struct DescriptorHandle{
	D3D12_CPU_DESCRIPTOR_HANDLE cpu {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu {};
	uint32_t offset = 0;

	bool IsValid() const{ return cpu.ptr != 0; }
};

enum class DescriptorUsage{
	CbvSrvUav,
	Rtv,
	Dsv,
	Sampler
};

struct DescriptorHeapSettings{
	UINT maxDescriptors = 60000;
	bool shaderVisible = true;
};

class DescriptorAllocator{
public:
	static void Initialize(ID3D12Device* device);
	static void CreateHeap(DescriptorUsage usage, const DescriptorHeapSettings& settings);

	static DescriptorHandle Allocate(DescriptorUsage usage);
	static void Free(DescriptorUsage usage, const DescriptorHandle& handle);

	static ID3D12DescriptorHeap* GetHeap(DescriptorUsage usage);
	static UINT GetDescriptorSize(DescriptorUsage usage);
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleStart(DescriptorUsage usage);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleStart(DescriptorUsage usage);
	static void Finalize();

private:
	struct HeapInfo{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
		UINT descriptorSize = 0;
		UINT currentOffset = 1; // reserve 0 for ImGui
		std::stack<UINT> freeList;
		std::mutex mutex;
		UINT maxDescriptors = 0;
	};

	static ID3D12Device* device_;
	static std::unordered_map<DescriptorUsage, HeapInfo> heaps_;
};