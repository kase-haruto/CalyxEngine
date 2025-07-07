#include "SrvLocator.h"

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> SrvLocator::srvHeap_ = nullptr;
uint32_t SrvLocator::descriptorSizeSrv_ = 0;
uint32_t SrvLocator::currentOffset_ = 0;
std::mutex SrvLocator::mutex_;
std::stack<uint32_t> SrvLocator::freeList_;

void SrvLocator::Provide(const ComPtr<ID3D12DescriptorHeap>& srvHeap, const ComPtr<ID3D12Device>& device){
	std::lock_guard<std::mutex> lock(mutex_);

	if (srvHeap_ != nullptr){
		throw std::runtime_error("SrvLocator already initialized.");
	}

	D3D12_DESCRIPTOR_HEAP_DESC desc = srvHeap->GetDesc();
	if (desc.Type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV){
		throw std::runtime_error("SrvLocator: Provided heap is not CBV/SRV/UAV type.");
	}

	srvHeap_ = srvHeap;
	descriptorSizeSrv_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	currentOffset_ = 1; // ImGuiが0番を使用済みと仮定
}

std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE> SrvLocator::AllocateSrv() {
	std::lock_guard<std::mutex> lock(mutex_);

	if (!srvHeap_) {
		throw std::runtime_error("SrvLocator has not been provided with a descriptor heap.");
	}

	uint32_t offset;
	if (!freeList_.empty()) {
		offset = freeList_.top();
		freeList_.pop();
	} else {
		CheckHeapAvailable(1);
		offset = currentOffset_++;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = srvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = srvHeap_->GetGPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += offset * descriptorSizeSrv_;
	gpuHandle.ptr += offset * descriptorSizeSrv_;
	return { cpuHandle, gpuHandle };
}


ComPtr<ID3D12DescriptorHeap> SrvLocator::GetSrvHeap() {
	std::lock_guard<std::mutex> lock(mutex_);
	return srvHeap_;
}

void SrvLocator::Finalize() {
	srvHeap_.Reset();
}

uint32_t SrvLocator::GetCurrentOffset() {
	std::lock_guard<std::mutex> lock(mutex_);
	return currentOffset_;
}


void SrvLocator::CheckHeapAvailable(uint32_t requested) {
	if (currentOffset_ + requested > kMaxSrvNum) {
		throw std::runtime_error("Not enough descriptors available in the heap.");
	}
}

void SrvLocator::FreeSrv(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
	std::lock_guard<std::mutex> lock(mutex_);

	if (!srvHeap_ || cpuHandle.ptr == 0) return;

	D3D12_CPU_DESCRIPTOR_HANDLE base = srvHeap_->GetCPUDescriptorHandleForHeapStart();
	size_t offsetBytes = cpuHandle.ptr - base.ptr;

	if (offsetBytes % descriptorSizeSrv_ != 0) {
		throw std::runtime_error("Invalid CPU handle offset");
	}

	uint32_t offset = static_cast<uint32_t>(offsetBytes / descriptorSizeSrv_);
	freeList_.push(offset);
}