#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include <mutex>
#include <stack>

/// <summary>
/// ハンドル
/// </summary>
struct DescriptorHandle{
	D3D12_CPU_DESCRIPTOR_HANDLE cpu {};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu {};
	uint32_t offset = 0;

	bool IsValid() const{ return cpu.ptr != 0; }
};

/// <summary>
/// 種類
/// </summary>
enum class DescriptorUsage{
	CbvSrvUav,
	Rtv,
	Dsv,
	Sampler
};

/// <summary>
/// ヒープ設定
/// </summary>
struct DescriptorHeapSettings{
	UINT maxDescriptors = 60000;
	bool shaderVisible = true;
};

/// <summary>
/// アロケータ
/// </summary>
class DescriptorAllocator{
public:
	static void Initialize(ID3D12Device* device);

	/// <summary>
	/// ヒープ作成
	/// </summary>
	/// <param name="usage"> ヒープ種類 </param>
	/// <param name="settings"> セッティング </param>
	static void CreateHeap(DescriptorUsage usage, const DescriptorHeapSettings& settings);

	/// <summary>
	/// ディスクリプタを確保
	/// </summary>
	/// <param name="usage"></param>
	/// <returns></returns>
	static DescriptorHandle Allocate(DescriptorUsage usage);

	/// <summary>
	/// 再利用できるようにリストに積む
	/// </summary>
	/// <param name="usage"></param>
	/// <param name="handle"></param>
	static void Free(DescriptorUsage usage, const DescriptorHandle& handle);

	/// <summary>
	/// 終了処理
	/// </summary>
	static void Finalize();

	//--------- accessor -----------------------------------------------------
	static ID3D12DescriptorHeap* GetHeap(DescriptorUsage usage);
	static UINT GetDescriptorSize(DescriptorUsage usage);
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleStart(DescriptorUsage usage);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleStart(DescriptorUsage usage);

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