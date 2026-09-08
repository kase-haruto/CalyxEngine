#include "TextRenderer.h"

#include <Engine/Graphics/Descriptor/DescriptorAllocator.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>
#include <Engine/Renderer/Text/GlyphAtlas.h>
#include <d3dx12.h>
#include <algorithm>
#include <cstring>

namespace CalyxEngine {
	bool TextRenderer::Initialize(ID3D12Device* device) {
		device_ = device;
		batches_.reserve(4);
		return device_ != nullptr;
	}

	void TextRenderer::Finalize() {
		if(vertexBuffer_ && mappedVertices_) vertexBuffer_->Unmap(0, nullptr);
		mappedVertices_ = nullptr;
		vertexBuffer_.Reset();
		vertexCapacity_ = 0;
		batches_.clear();
		device_ = nullptr;
	}

	bool TextRenderer::EnsureVertexCapacity(size_t vertexCount) {
		if(vertexCount <= vertexCapacity_) return true;
		const size_t newCapacity = (std::max)(vertexCount, (std::max)(vertexCapacity_ * 2, size_t{1024}));
		if(vertexBuffer_ && mappedVertices_) vertexBuffer_->Unmap(0, nullptr);
		vertexBuffer_.Reset();
		mappedVertices_ = nullptr;
		CD3DX12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(Vertex) * newCapacity);
		if(FAILED(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_)))) return false;
		if(FAILED(vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices_)))) {
			vertexBuffer_.Reset();
			return false;
		}
		vertexCapacity_ = newCapacity;
		return true;
	}

	void TextRenderer::Draw(ID3D12GraphicsCommandList* commandList, PipelineService* pipelines,
		const GlyphAtlas& atlas, const std::vector<TextDrawData>& draws,
		uint32_t viewportWidth, uint32_t viewportHeight) {
		if(!commandList || !pipelines || viewportWidth == 0 || viewportHeight == 0) return;
		if(batches_.size() < atlas.GetPageCount()) batches_.resize(atlas.GetPageCount());
		for(uint32_t page = 0; page < atlas.GetPageCount(); ++page) {
			batches_[page].page_ = page;
			batches_[page].vertices_.clear();
		}

		for(const TextDrawData& draw : draws) {
			if(!draw.layout_) continue;
			for(size_t i = 0; i < draw.layout_->glyphs_.size(); ++i) {
				const PositionedGlyph& positioned = draw.layout_->glyphs_[i];
				// sourceIndex includes spaces and line breaks, allowing typewriter timing to
				// advance by Unicode code point rather than only by drawable glyph count.
				if(static_cast<size_t>(positioned.sourceIndex_) >= draw.visibleGlyphCount_) break;
				const GlyphInfo& glyph = positioned.glyph_;
				if(glyph.atlasPage_ >= batches_.size()) continue;
				const float left = positioned.position_.x;
				const float top = positioned.position_.y;
				const float right = left + glyph.size_.x;
				const float bottom = top + glyph.size_.y;
				auto& vertices = batches_[glyph.atlasPage_].vertices_;
				vertices.insert(vertices.end(), {
					{{left, top}, glyph.uvMin_, draw.color_},
					{{right, top}, {glyph.uvMax_.x, glyph.uvMin_.y}, draw.color_},
					{{left, bottom}, {glyph.uvMin_.x, glyph.uvMax_.y}, draw.color_},
					{{right, top}, {glyph.uvMax_.x, glyph.uvMin_.y}, draw.color_},
					{{right, bottom}, glyph.uvMax_, draw.color_},
					{{left, bottom}, {glyph.uvMin_.x, glyph.uvMax_.y}, draw.color_}});
			}
		}

		size_t totalVertices = 0;
		for(size_t page = 0; page < atlas.GetPageCount(); ++page) totalVertices += batches_[page].vertices_.size();
		if(totalVertices == 0 || !EnsureVertexCapacity(totalVertices)) return;
		size_t offset = 0;
		for(size_t page = 0; page < atlas.GetPageCount(); ++page) {
			const auto& batch = batches_[page];
			if(batch.vertices_.empty()) continue;
			std::memcpy(mappedVertices_ + offset, batch.vertices_.data(), batch.vertices_.size() * sizeof(Vertex));
			offset += batch.vertices_.size();
		}

		pipelines->SetCommand(pipelines->GetPipelineSet(PipelineTag::Object::Text, BlendMode::NONE), commandList);
		ID3D12DescriptorHeap* heap = DescriptorAllocator::GetHeap(DescriptorUsage::CbvSrvUav);
		commandList->SetDescriptorHeaps(1, &heap);
		const float viewport[2] = {static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
		commandList->SetGraphicsRoot32BitConstants(0, 2, viewport, 0);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		offset = 0;
		for(size_t page = 0; page < atlas.GetPageCount(); ++page) {
			const auto& batch = batches_[page];
			if(batch.vertices_.empty()) continue;
			D3D12_VERTEX_BUFFER_VIEW view{};
			view.BufferLocation = vertexBuffer_->GetGPUVirtualAddress() + offset * sizeof(Vertex);
			view.SizeInBytes = static_cast<UINT>(batch.vertices_.size() * sizeof(Vertex));
			view.StrideInBytes = sizeof(Vertex);
			commandList->IASetVertexBuffers(0, 1, &view);
			commandList->SetGraphicsRootDescriptorTable(1, atlas.GetPageSrv(batch.page_));
			commandList->DrawInstanced(static_cast<UINT>(batch.vertices_.size()), 1, 0, 0);
			offset += batch.vertices_.size();
		}
	}
}
