#pragma once

#include <Engine/Foundation/Export/CalyxAPI.h>
/* ========================================================================
/*  include space
/* ===================================================================== */
#include <d3d12.h>
#include <mutex>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>
#include <wrl.h>

/*----------------------------------------------------------------
 * Descriptor Handle
 *	- ディスクリプタハンドル
 *--------------------------------------------------------------*/
/**
 * @brief DescriptorHandleに関するデータを保持する構造体です。
 */
struct DescriptorHandle {
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
	uint32_t					offset = 0;

	bool IsValid() const { return cpu.ptr != 0; }
};

/*----------------------------------------------------------------
 * Descriptor Usage
 *	- ディスクリプタの使用用途
 *--------------------------------------------------------------*/
enum class DescriptorUsage {
	CbvSrvUav,
	Rtv,
	Dsv,
	Sampler
};

/*----------------------------------------------------------------
 * Descriptor Heap Settings
 *	- ディスクリプタヒープ生成時の設定
 *--------------------------------------------------------------*/
/**
 * @brief DescriptorHeapSettingsに関するデータを保持する構造体です。
 */
struct DescriptorHeapSettings {
	UINT maxDescriptors = 60000;
	bool shaderVisible	= true;
};

/*----------------------------------------------------------------
 * Descriptor Allocator
 *	- ディスクリプタヒープの管理・割り当てを行うクラス
 *--------------------------------------------------------------*/
/**
 * @brief DescriptorAllocatorの機能を提供するクラスです。
 */
class CALYX_API DescriptorAllocator {
public:
	/**
	 * \brief 初期化
	 * \param device
	 */
	static void Initialize(ID3D12Device* device);
	/**
	 * \brief 終了処理
	 */
	static void Finalize();
	/**
	 * \brief ヒープの生成
	 * \param usage
	 * \param settings
	 */
	static void CreateHeap(DescriptorUsage usage, const DescriptorHeapSettings& settings);
	/**
	 * \brief ディスクリプタの割り当て
	 * \param usage
	 * \return 割り当てたディスクリプタハンドル
	 */
	static DescriptorHandle Allocate(DescriptorUsage usage);
	static DescriptorHandle AllocateRange(DescriptorUsage usage, UINT count);
	/*	 * \brief ディスクリプタの解放
	 * \param usage
	 * \param handle
	 */
	static void Free(DescriptorUsage usage, const DescriptorHandle& handle);
	static void FreeRange(DescriptorUsage usage, const DescriptorHandle& handle, UINT count);

	//--------- accessor -----------------------------------------------------
	static ID3D12DescriptorHeap*	   GetHeap(DescriptorUsage usage);
	static UINT						   GetDescriptorSize(DescriptorUsage usage);
	static D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleStart(DescriptorUsage usage);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleStart(DescriptorUsage usage);

private:
	/**
	 * @brief HeapInfoに関するデータを保持する構造体です。
	 */
	struct HeapInfo {
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
		UINT										 descriptorSize = 0;
		UINT										 currentOffset	= 1; // reserve 0 for ImGui
		std::stack<UINT>							 freeList;
		std::vector<std::pair<UINT, UINT>>			 freeRanges;
		std::mutex									 mutex;
		UINT										 maxDescriptors = 0;
		bool shaderVisible = false;
	};

	static DescriptorHandle MakeHandle(const HeapInfo& info, UINT offset);
	static void AddFreeRange(HeapInfo& info, UINT offset, UINT count);

	static ID3D12Device*								 device_;
	static std::unordered_map<DescriptorUsage, HeapInfo> heaps_;
};
