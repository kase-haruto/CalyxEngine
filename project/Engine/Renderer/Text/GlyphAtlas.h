#pragma once

#include <Engine/Assets/Font/FontTypes.h>
#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>

#include <d3d12.h>
#include <memory>
#include <vector>
#include <wrl.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * ShelfAtlasPacker
	 * - Shelf方式でAtlas上の空き矩形を確保する
	 * - Glyph生成とGPU転送は担当しない
	 *---------------------------------------------------------------------------------------*/
	class ShelfAtlasPacker {
	public:
		ShelfAtlasPacker(uint32_t width, uint32_t height, uint32_t padding = 1)
			: width_(width), height_(height), padding_(padding) {}
		[[nodiscard]] bool Allocate(uint32_t width, uint32_t height, uint32_t& x, uint32_t& y);
		[[nodiscard]] uint64_t GetUsedPixels() const noexcept { return usedPixels_; }

	private:
		uint32_t width_ = 0, height_ = 0, padding_ = 1;
		uint32_t cursorX_ = 0, cursorY_ = 0, shelfHeight_ = 0;
		uint64_t usedPixels_ = 0;
	};

	class GlyphAtlas {
	public:
		static constexpr uint32_t kPageSize = 1024;

		GlyphAtlas() = default;
		~GlyphAtlas();
		bool Initialize(ID3D12Device* device);
		void Finalize();
		void BeginFrame();
		[[nodiscard]] bool Insert(const RasterizedGlyph& bitmap, GlyphInfo& output);
		void UploadDirtyPages(ID3D12GraphicsCommandList* commandList);
		[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetPageSrv(uint32_t page) const;
		[[nodiscard]] size_t GetPageCount() const noexcept { return pages_.size(); }
		[[nodiscard]] uint64_t GetUsedPixels() const noexcept;

	private:
		struct Page {
			Microsoft::WRL::ComPtr<ID3D12Resource> texture_;
			DescriptorHandle srv_{};
			ShelfAtlasPacker packer_{kPageSize, kPageSize};
			std::vector<uint8_t> pixels_;
			bool dirty_ = false;
		};

		[[nodiscard]] Page* CreatePage();
		ID3D12Device* device_ = nullptr;
		std::vector<std::unique_ptr<Page>> pages_;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> frameUploads_;
	};
}
