#include "GlyphAtlas.h"

#include <Engine/Foundation/Log/EngineLogger.h>
#include <d3dx12.h>
#include <algorithm>
#include <cstring>

namespace CalyxEngine {

	bool ShelfAtlasPacker::Allocate(uint32_t width, uint32_t height, uint32_t& x, uint32_t& y) {
		if(width == 0 || height == 0 || width + padding_ * 2 > width_ || height + padding_ * 2 > height_) return false;
		const uint32_t paddedWidth = width + padding_ * 2;
		const uint32_t paddedHeight = height + padding_ * 2;
		if(cursorX_ + paddedWidth > width_) {
			cursorX_ = 0;
			cursorY_ += shelfHeight_;
			shelfHeight_ = 0;
		}
		if(cursorY_ + paddedHeight > height_) return false;
		x = cursorX_ + padding_;
		y = cursorY_ + padding_;
		cursorX_ += paddedWidth;
		shelfHeight_ = (std::max)(shelfHeight_, paddedHeight);
		usedPixels_ += static_cast<uint64_t>(paddedWidth) * paddedHeight;
		return true;
	}

	GlyphAtlas::~GlyphAtlas() { Finalize(); }

	bool GlyphAtlas::Initialize(ID3D12Device* device) {
		device_ = device;
		return device_ != nullptr;
	}

	void GlyphAtlas::Finalize() {
		frameUploads_.clear();
		for(auto& page : pages_) DescriptorAllocator::Free(DescriptorUsage::CbvSrvUav, page->srv_);
		pages_.clear();
		device_ = nullptr;
	}

	void GlyphAtlas::BeginFrame() {
		// DxCoreは前フレーム末尾でFence完了を待つため、ここでUpload Resourceを安全に解放できる。
		frameUploads_.clear();
	}

	GlyphAtlas::Page* GlyphAtlas::CreatePage() {
		if(!device_) return nullptr;
		auto page = std::make_unique<Page>();
		page->pixels_.resize(static_cast<size_t>(kPageSize) * kPageSize, 0);

		D3D12_RESOURCE_DESC desc{};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Width = kPageSize;
		desc.Height = kPageSize;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_DEFAULT);
		if(FAILED(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&page->texture_)))) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Rendering,
				"Failed to create glyph atlas texture.", "GlyphAtlas");
			return nullptr;
		}

		try {
			page->srv_ = DescriptorAllocator::Allocate(DescriptorUsage::CbvSrvUav);
		} catch(const std::exception& error) {
			EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Rendering, error.what(), "GlyphAtlas");
			return nullptr;
		}
		D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
		srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv.Format = DXGI_FORMAT_R8_UNORM;
		srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv.Texture2D.MipLevels = 1;
		device_->CreateShaderResourceView(page->texture_.Get(), &srv, page->srv_.cpu);
		pages_.push_back(std::move(page));
		return pages_.back().get();
	}

	bool GlyphAtlas::Insert(const RasterizedGlyph& bitmap, GlyphInfo& output) {
		output.size_ = {static_cast<float>(bitmap.width_), static_cast<float>(bitmap.height_)};
		output.bearing_ = {static_cast<float>(bitmap.bearingX_), static_cast<float>(bitmap.bearingY_)};
		output.advance_ = bitmap.advance_;
		if(bitmap.width_ == 0 || bitmap.height_ == 0) return true;

		uint32_t x = 0, y = 0;
		Page* selected = nullptr;
		uint32_t pageIndex = 0;
		for(uint32_t i = 0; i < pages_.size(); ++i) {
			if(pages_[i]->packer_.Allocate(bitmap.width_, bitmap.height_, x, y)) {
				selected = pages_[i].get();
				pageIndex = i;
				break;
			}
		}
		if(!selected) {
			selected = CreatePage();
			pageIndex = static_cast<uint32_t>(pages_.size() - 1);
			if(!selected || !selected->packer_.Allocate(bitmap.width_, bitmap.height_, x, y)) {
				EngineLogger::GetInstance().Add(LogLevel::Error, LogCategory::Rendering,
					"Glyph is larger than an atlas page.", "GlyphAtlas");
				return false;
			}
		}

		for(uint32_t row = 0; row < bitmap.height_; ++row) {
			std::memcpy(selected->pixels_.data() + static_cast<size_t>(y + row) * kPageSize + x,
				bitmap.pixels_.data() + static_cast<size_t>(row) * bitmap.width_, bitmap.width_);
		}
		selected->dirty_ = true;
		output.atlasPage_ = pageIndex;
		output.uvMin_ = {static_cast<float>(x) / kPageSize, static_cast<float>(y) / kPageSize};
		output.uvMax_ = {static_cast<float>(x + bitmap.width_) / kPageSize,
			static_cast<float>(y + bitmap.height_) / kPageSize};
		return true;
	}

	void GlyphAtlas::UploadDirtyPages(ID3D12GraphicsCommandList* commandList) {
		for(auto& page : pages_) {
			if(!page->dirty_) continue;
			const auto textureDesc = page->texture_->GetDesc();
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
			UINT rows = 0;
			UINT64 rowSize = 0, totalBytes = 0;
			device_->GetCopyableFootprints(&textureDesc, 0, 1, 0, &footprint, &rows, &rowSize, &totalBytes);
			Microsoft::WRL::ComPtr<ID3D12Resource> upload;
			CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);
			if(FAILED(device_->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&upload)))) continue;

			uint8_t* mapped = nullptr;
			if(FAILED(upload->Map(0, nullptr, reinterpret_cast<void**>(&mapped)))) continue;
			for(UINT row = 0; row < rows; ++row) {
				std::memcpy(mapped + footprint.Offset + static_cast<size_t>(row) * footprint.Footprint.RowPitch,
					page->pixels_.data() + static_cast<size_t>(row) * kPageSize, static_cast<size_t>(rowSize));
			}
			upload->Unmap(0, nullptr);

			CD3DX12_RESOURCE_BARRIER toCopy = CD3DX12_RESOURCE_BARRIER::Transition(page->texture_.Get(),
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
			commandList->ResourceBarrier(1, &toCopy);
			CD3DX12_TEXTURE_COPY_LOCATION destination(page->texture_.Get(), 0);
			CD3DX12_TEXTURE_COPY_LOCATION source(upload.Get(), footprint);
			commandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
			CD3DX12_RESOURCE_BARRIER toShader = CD3DX12_RESOURCE_BARRIER::Transition(page->texture_.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			commandList->ResourceBarrier(1, &toShader);
			frameUploads_.push_back(std::move(upload));
			page->dirty_ = false;
		}
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GlyphAtlas::GetPageSrv(uint32_t page) const {
		return page < pages_.size() ? pages_[page]->srv_.gpu : D3D12_GPU_DESCRIPTOR_HANDLE{};
	}

	uint64_t GlyphAtlas::GetUsedPixels() const noexcept {
		uint64_t result = 0;
		for(const auto& page : pages_) result += page->packer_.GetUsedPixels();
		return result;
	}
}
