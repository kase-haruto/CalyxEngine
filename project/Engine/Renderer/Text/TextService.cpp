#include "TextService.h"

#include <Engine/Assets/Font/FontRepository.h>
#include <Engine/Foundation/Log/EngineLogger.h>
#include <Engine/Foundation/Utility/Text/Utf8Decoder.h>
#include <Engine/Renderer/Text/GlyphAtlas.h>
#include <Engine/Renderer/Text/GlyphCache.h>
#include <Engine/Renderer/Text/TextLayout.h>
#include <Engine/Renderer/Text/TextRenderer.h>

#include <algorithm>
#include <limits>
#include <vector>

namespace CalyxEngine {
	struct TextService::Impl {
		struct Request {
			FontHandle font_{};
			std::u8string text_;
			Vector2 position_{};
			TextStyle style_{};
			size_t visibleGlyphCount_ = (std::numeric_limits<size_t>::max)();
		};

		std::unique_ptr<FontRepository> fonts_;
		std::unique_ptr<GlyphAtlas> atlas_;
		std::unique_ptr<GlyphCache> cache_;
		std::unique_ptr<TextLayout> layout_;
		std::unique_ptr<TextRenderer> renderer_;
		std::vector<Request> requests_;
		std::vector<TextLayoutResult> layouts_;
		std::vector<TextDrawData> draws_;
		bool prepared_ = false;
	};

	TextService::TextService() = default;
	TextService::~TextService() = default;

	TextService* TextService::GetInstance() {
		static TextService instance;
		return &instance;
	}

	bool TextService::Initialize(ID3D12Device* device) {
		if(impl_) return true;
		auto state = std::make_unique<Impl>();
		state->fonts_ = std::make_unique<FontRepository>();
		state->atlas_ = std::make_unique<GlyphAtlas>();
		state->layout_ = std::make_unique<TextLayout>();
		state->renderer_ = std::make_unique<TextRenderer>();
		if(!state->atlas_->Initialize(device) || !state->renderer_->Initialize(device)) return false;
		state->cache_ = std::make_unique<GlyphCache>(*state->fonts_, *state->atlas_);
		state->requests_.reserve(64);
		state->layouts_.reserve(64);
		state->draws_.reserve(64);
		impl_ = std::move(state);
		return true;
	}

	void TextService::Finalize() {
		if(!impl_) return;
		impl_->renderer_->Finalize();
		impl_->atlas_->Finalize();
		impl_.reset();
	}

	void TextService::BeginFrame() {
		if(!impl_) return;
		impl_->atlas_->BeginFrame();
		impl_->requests_.clear();
		impl_->layouts_.clear();
		impl_->draws_.clear();
		impl_->prepared_ = false;
	}

	FontHandle TextService::LoadFont(const std::string& path) {
		return impl_ ? impl_->fonts_->Load(path) : FontHandle{};
	}

	void TextService::Draw(FontHandle font, std::u8string_view text,
		const Vector2& position, const TextStyle& style) {
		Draw(font, text, position, style, (std::numeric_limits<size_t>::max)());
	}

	void TextService::Draw(FontHandle font, std::u8string_view text, const Vector2& position,
		const TextStyle& style, size_t visibleGlyphCount) {
		if(!impl_ || !font || text.empty()) return;
		impl_->requests_.push_back({font, std::u8string(text), position, style, visibleGlyphCount});
		impl_->prepared_ = false;
	}

	void TextService::Render(ID3D12GraphicsCommandList* commandList, PipelineService* pipelines,
		uint32_t viewportWidth, uint32_t viewportHeight) {
		if(!impl_ || impl_->requests_.empty()) return;
		if(!impl_->prepared_) {
			impl_->layouts_.clear();
			impl_->draws_.clear();
			impl_->layouts_.reserve(impl_->requests_.size());
			impl_->draws_.reserve(impl_->requests_.size());
			for(const auto& request : impl_->requests_) {
				const auto codePoints = Utf8Decoder::Decode(request.text_);
				impl_->layouts_.push_back(impl_->layout_->Layout(
					request.font_, codePoints, request.position_, request.style_, *impl_->cache_));
			}
			for(size_t i = 0; i < impl_->requests_.size(); ++i) {
				impl_->draws_.push_back({&impl_->layouts_[i], impl_->requests_[i].style_.color_,
					impl_->requests_[i].visibleGlyphCount_});
			}
			impl_->prepared_ = true;
		}
		impl_->atlas_->UploadDirtyPages(commandList);
		impl_->renderer_->Draw(commandList, pipelines, *impl_->atlas_, impl_->draws_, viewportWidth, viewportHeight);
	}

	TextDebugStats TextService::GetDebugStats() const {
		if(!impl_) return {};
		const size_t pages = impl_->atlas_->GetPageCount();
		const uint64_t capacity = static_cast<uint64_t>(pages) * GlyphAtlas::kPageSize * GlyphAtlas::kPageSize;
		return {impl_->fonts_->GetFontCount(), impl_->cache_->GetGlyphCount(), pages, GlyphAtlas::kPageSize,
			capacity ? static_cast<float>(impl_->atlas_->GetUsedPixels()) / static_cast<float>(capacity) : 0.0f};
	}
}
