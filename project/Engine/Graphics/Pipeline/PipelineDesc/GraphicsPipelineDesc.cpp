#include "GraphicsPipelineDesc.h"
/* ========================================================================
/* include space
/* ===================================================================== */

/////////////////////////////////////////////////////////////////////////////////////////
//		hashの合成
/////////////////////////////////////////////////////////////////////////////////////////
namespace {
	void HashCombine(std::size_t& seed, std::size_t value) noexcept {
		seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		shaderPath
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc& GraphicsPipelineDesc::VS(const std::wstring& path) { vs_ = path; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::PS(const std::wstring& path) { ps_ = path; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::GS(const std::wstring& path) { gs_ = path; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::CS(const std::wstring& path) { cs_ = path;isCompute_ = true; return *this; }

/////////////////////////////////////////////////////////////////////////////////////////
//		input / RTV / sample
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc& GraphicsPipelineDesc::Input(const std::vector<D3D12_INPUT_ELEMENT_DESC>& element) { inputElems_ = element; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::RTV(DXGI_FORMAT format) { rtvFormats_ = { format }; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::Samples(UINT count) { sampleCount_ = count; return *this; }

GraphicsPipelineDesc& GraphicsPipelineDesc::StaticSamplerWrapLinear(UINT shaderRegister) {
	root_.SamplerWrapLinear(shaderRegister, D3D12_SHADER_VISIBILITY_PIXEL);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		rasterizer
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc& GraphicsPipelineDesc::Rasterizer(const D3D12_RASTERIZER_DESC& rasterizer) { rasterizer_ = rasterizer; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::CullMode(D3D12_CULL_MODE mode) { rasterizer_.CullMode = mode; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::FillMode(D3D12_FILL_MODE mode) { rasterizer_.FillMode = mode; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::Wireframe() { rasterizer_.FillMode = D3D12_FILL_MODE_WIREFRAME; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::CullNone() { rasterizer_.CullMode = D3D12_CULL_MODE_NONE; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::CullBack() { rasterizer_.CullMode = D3D12_CULL_MODE_BACK; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::CullFront() { rasterizer_.CullMode = D3D12_CULL_MODE_FRONT; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::DepthBias(INT bias) { rasterizer_.DepthBias = bias; return *this; }

/////////////////////////////////////////////////////////////////////////////////////////
//		blend
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc& GraphicsPipelineDesc::BlendState(const D3D12_BLEND_DESC& blend) { blend_ = blend; return *this; }
GraphicsPipelineDesc& GraphicsPipelineDesc::Blend(BlendMode mode) { blend_ = CreateBlendDesc(mode); return *this; }

GraphicsPipelineDesc& GraphicsPipelineDesc::BlendAlpha() { return Blend(BlendMode::ALPHA); }
GraphicsPipelineDesc& GraphicsPipelineDesc::BlendAdditive() { return Blend(BlendMode::ADD); }
GraphicsPipelineDesc& GraphicsPipelineDesc::BlendNone() { return Blend(BlendMode::NONE); }
GraphicsPipelineDesc& GraphicsPipelineDesc::BlendSub() { return Blend(BlendMode::SUB); }
GraphicsPipelineDesc& GraphicsPipelineDesc::BlendMul() { return Blend(BlendMode::MUL); }
GraphicsPipelineDesc& GraphicsPipelineDesc::BlendScreen() { return Blend(BlendMode::SCREEN); }

/////////////////////////////////////////////////////////////////////////////////////////
//		depth
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc& GraphicsPipelineDesc::DepthState(const D3D12_DEPTH_STENCIL_DESC& depth) { depth_ = depth; return *this; }

GraphicsPipelineDesc& GraphicsPipelineDesc::DepthEnable(bool on) {
	depth_.DepthEnable = on;
	depth_.DepthWriteMask = on ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	return *this;
}

GraphicsPipelineDesc& GraphicsPipelineDesc::DepthFunc(D3D12_COMPARISON_FUNC func) { depth_.DepthFunc = func; return *this; }

/////////////////////////////////////////////////////////////////////////////////////////
//		operator/hash
/////////////////////////////////////////////////////////////////////////////////////////
bool GraphicsPipelineDesc::operator==(const GraphicsPipelineDesc& o) const noexcept {
	return vs_ == o.vs_ && ps_ == o.ps_ && gs_ == o.gs_ && cs_ == o.cs_ &&
		   isCompute_ == o.isCompute_ &&
		   dsvFormat_ == o.dsvFormat_ &&
		   rasterizer_.CullMode == o.rasterizer_.CullMode &&
		   rasterizer_.FillMode == o.rasterizer_.FillMode &&
		   blend_.RenderTarget[0].SrcBlend == o.blend_.RenderTarget[0].SrcBlend &&
		   blend_.RenderTarget[0].DestBlend == o.blend_.RenderTarget[0].DestBlend &&
		   depth_.DepthEnable == o.depth_.DepthEnable &&
		   depth_.DepthWriteMask == o.depth_.DepthWriteMask &&
		   depth_.DepthFunc == o.depth_.DepthFunc &&
		   sampleCount_ == o.sampleCount_ &&
		   rtvFormats_ == o.rtvFormats_ &&
		   inputElems_.size() == o.inputElems_.size() && 
		   root_.Hash() == o.root_.Hash();
}


size_t GraphicsPipelineDesc::Hash() const noexcept {
	size_t h = 0;
	HashCombine(h, std::hash<std::wstring>{}(vs_));
	HashCombine(h, std::hash<std::wstring>{}(ps_));
	HashCombine(h, std::hash<std::wstring>{}(gs_));
	HashCombine(h, std::hash<std::wstring>{}(cs_));
	HashCombine(h, static_cast<size_t>(isCompute_));

	HashCombine(h, dsvFormat_);

	HashCombine(h, rasterizer_.CullMode);
	HashCombine(h, rasterizer_.FillMode);

	HashCombine(h, blend_.RenderTarget[0].SrcBlend);
	HashCombine(h, blend_.RenderTarget[0].DestBlend);

	HashCombine(h, depth_.DepthEnable);
	HashCombine(h, depth_.DepthWriteMask);
	HashCombine(h, depth_.DepthFunc);

	HashCombine(h, sampleCount_);

	HashCombine(h, rtvFormats_.size());
	for(auto f : rtvFormats_) HashCombine(h, f);

	HashCombine(h, root_.Hash());

	HashCombine(h, inputElems_.size());

	return h;
}


