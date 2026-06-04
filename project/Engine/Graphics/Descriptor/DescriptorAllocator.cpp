#include "DescriptorAllocator.h"
#include <algorithm>
#include <stdexcept>

ID3D12Device* DescriptorAllocator::device_ = nullptr;
std::unordered_map<DescriptorUsage, DescriptorAllocator::HeapInfo> DescriptorAllocator::heaps_;

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void DescriptorAllocator::Initialize(ID3D12Device* device){
	device_ = device;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ヒープ作成
/////////////////////////////////////////////////////////////////////////////////////////
void DescriptorAllocator::CreateHeap(DescriptorUsage usage, const DescriptorHeapSettings& settings){
	if (!device_) throw std::runtime_error("DescriptorAllocator: device not initialized.");

	D3D12_DESCRIPTOR_HEAP_DESC desc {};
	switch (usage){
		case DescriptorUsage::CbvSrvUav: desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; break;
		case DescriptorUsage::Rtv:       desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; break;
		case DescriptorUsage::Dsv:       desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; break;
		case DescriptorUsage::Sampler:   desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER; break;
	}
	desc.NumDescriptors = settings.maxDescriptors;
	desc.Flags = settings.shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 0;

	HeapInfo& info = heaps_[usage];
	HRESULT hr = device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&info.heap));
	if (FAILED(hr)) throw std::runtime_error("DescriptorAllocator: failed to create descriptor heap.");

	info.descriptorSize = device_->GetDescriptorHandleIncrementSize(desc.Type);
	info.currentOffset = (usage == DescriptorUsage::CbvSrvUav) ? 1 : 0; // Descriptor 0 is reserved for ImGui font
	info.maxDescriptors = settings.maxDescriptors;
	info.shaderVisible = settings.shaderVisible;
	info.freeList = {};
	info.freeRanges.clear();
}

DescriptorHandle DescriptorAllocator::MakeHandle(const HeapInfo& info, UINT offset) {
	DescriptorHandle handle{};
	handle.offset = offset;

	auto cpuStart = info.heap->GetCPUDescriptorHandleForHeapStart();
	handle.cpu.ptr = cpuStart.ptr + offset * info.descriptorSize;

	if(info.shaderVisible) {
		auto gpuStart = info.heap->GetGPUDescriptorHandleForHeapStart();
		handle.gpu.ptr = gpuStart.ptr + offset * info.descriptorSize;
	} else {
		handle.gpu.ptr = 0;
	}

	return handle;
}

void DescriptorAllocator::AddFreeRange(HeapInfo& info, UINT offset, UINT count) {
	if(count == 0) return;

	info.freeRanges.emplace_back(offset, count);
	std::sort(info.freeRanges.begin(), info.freeRanges.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.first < rhs.first;
	});

	std::vector<std::pair<UINT, UINT>> merged;
	merged.reserve(info.freeRanges.size());
	for(const auto& range : info.freeRanges) {
		if(merged.empty()) {
			merged.push_back(range);
			continue;
		}

		auto& back = merged.back();
		const UINT backEnd = back.first + back.second;
		if(range.first <= backEnd) {
			const UINT rangeEnd = range.first + range.second;
			back.second = (std::max)(backEnd, rangeEnd) - back.first;
		} else {
			merged.push_back(range);
		}
	}
	info.freeRanges = std::move(merged);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		渡す
/////////////////////////////////////////////////////////////////////////////////////////
DescriptorHandle DescriptorAllocator::Allocate(DescriptorUsage usage){
	HeapInfo&					info = heaps_[usage];
	std::lock_guard<std::mutex> lock(info.mutex);

	UINT offset = 0;

	if(!info.freeList.empty()) {
		offset = info.freeList.top();
		info.freeList.pop();
	} else if(!info.freeRanges.empty()) {
		auto& range = info.freeRanges.back();
		offset = range.first;
		++range.first;
		--range.second;
		if(range.second == 0) {
			info.freeRanges.pop_back();
		}
	} else {
		if(info.currentOffset >= info.maxDescriptors)
			throw std::runtime_error("DescriptorAllocator: Heap is full");
		offset = info.currentOffset++;
	}

	return MakeHandle(info, offset);
}

DescriptorHandle DescriptorAllocator::AllocateRange(DescriptorUsage usage, UINT count) {
	if(count == 0) return {};
	HeapInfo& info = heaps_[usage];
	std::lock_guard<std::mutex> lock(info.mutex);

	for(auto it = info.freeRanges.begin(); it != info.freeRanges.end(); ++it) {
		if(it->second < count) continue;

		const UINT offset = it->first;
		it->first += count;
		it->second -= count;
		if(it->second == 0) {
			info.freeRanges.erase(it);
		}

		return MakeHandle(info, offset);
	}

	if(info.currentOffset + count > info.maxDescriptors) {
		throw std::runtime_error("DescriptorAllocator: Heap is full");
	}

	const UINT offset = info.currentOffset;
	info.currentOffset += count;

	return MakeHandle(info, offset);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////////////////
void DescriptorAllocator::Free(DescriptorUsage usage, const DescriptorHandle& handle){
	if (!handle.IsValid()) return;
	auto heapIt = heaps_.find(usage);
	if(heapIt == heaps_.end()) return;
	HeapInfo& info = heapIt->second;
	std::lock_guard<std::mutex> lock(info.mutex);
	info.freeList.push(handle.offset);
}

void DescriptorAllocator::FreeRange(DescriptorUsage usage, const DescriptorHandle& handle, UINT count) {
	if(!handle.IsValid() || count == 0) return;
	auto heapIt = heaps_.find(usage);
	if(heapIt == heaps_.end()) return;
	HeapInfo& info = heapIt->second;
	std::lock_guard<std::mutex> lock(info.mutex);
	AddFreeRange(info, handle.offset, count);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ヒープ取得
/////////////////////////////////////////////////////////////////////////////////////////
ID3D12DescriptorHeap* DescriptorAllocator::GetHeap(DescriptorUsage usage){
	return heaps_[usage].heap.Get();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ディスクリプタのサイズ取得
/////////////////////////////////////////////////////////////////////////////////////////
UINT DescriptorAllocator::GetDescriptorSize(DescriptorUsage usage){
	return heaps_[usage].descriptorSize;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetCpuHandleStart(DescriptorUsage usage){
	return heaps_[usage].heap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetGpuHandleStart(DescriptorUsage usage){
	return heaps_[usage].heap->GetGPUDescriptorHandleForHeapStart();
}


/////////////////////////////////////////////////////////////////////////////////////////
//		解放
/////////////////////////////////////////////////////////////////////////////////////////
void DescriptorAllocator::Finalize(){
	heaps_.clear();
	device_ = nullptr;
}
