#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/graphics/Pipeline/RootSig/RootSignatureBuilder.h>
#include <Engine/graphics/Pipeline/BlendMode/BlendMode.h>

// c++
#include <string>
#include <vector>
#include <d3d12.h>

/* ========================================================================
/* パイプラインですく
/* ===================================================================== */
struct GraphicsPipelineDesc{
public:
	//===================================================================*/
	//		functions
	//===================================================================*/
	// shaders ----------------------------------------------------------//
	GraphicsPipelineDesc& VS(const std::wstring& path);
	GraphicsPipelineDesc& PS(const std::wstring& path);
	GraphicsPipelineDesc& CS(const std::wstring& path);

	// IA / RT ----------------------------------------------------------//
	GraphicsPipelineDesc& Input(const std::vector<D3D12_INPUT_ELEMENT_DESC>& e);
	GraphicsPipelineDesc& RTV(DXGI_FORMAT format);
	GraphicsPipelineDesc& Samples(UINT count);
	GraphicsPipelineDesc& StaticSamplerWrapLinear(UINT shaderRegister);

	// rasterizer -------------------------------------------------------//
	GraphicsPipelineDesc& Rasterizer(const D3D12_RASTERIZER_DESC& r);
	GraphicsPipelineDesc& CullMode(D3D12_CULL_MODE mode);
	GraphicsPipelineDesc& FillMode(D3D12_FILL_MODE mode);
	GraphicsPipelineDesc& Wireframe();
	GraphicsPipelineDesc& CullNone();
	GraphicsPipelineDesc& CullBack();
	GraphicsPipelineDesc& CullFront();

	// blend ------------------------------------------------------------//
	GraphicsPipelineDesc& BlendState(const D3D12_BLEND_DESC& blend);
	GraphicsPipelineDesc& Blend(BlendMode mode);

	GraphicsPipelineDesc& BlendAlpha();
	GraphicsPipelineDesc& BlendAdditive();
	GraphicsPipelineDesc& BlendNone();
	GraphicsPipelineDesc& BlendSub();
	GraphicsPipelineDesc& BlendMul();
	GraphicsPipelineDesc& BlendScreen();

	// depth ------------------------------------------------------------//
	GraphicsPipelineDesc& DepthState(const D3D12_DEPTH_STENCIL_DESC& depth);
	GraphicsPipelineDesc& DepthEnable(bool on);
	GraphicsPipelineDesc& DepthFunc(D3D12_COMPARISON_FUNC func);

public:
	//===================================================================*/
	//		variables
	//===================================================================*/
	// shaderPath -------------------------------------------------------//
	std::wstring vs_;		//< vertex		shader path
	std::wstring ps_;		//< pixel		shader path
	std::wstring cs_;		//< compute		shader path
	bool isCompute_ = false;

	// rootSignature ---------------------------------------------------//
	RootSignatureBuilder root_;	//< root signature

	// input layout ----------------------------------------------------//
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElems_;

	// render target format --------------------------------------------//
	DXGI_FORMAT dsvFormat_ = DXGI_FORMAT_D24_UNORM_S8_UINT;
	std::vector<DXGI_FORMAT>  rtvFormats_ = {DXGI_FORMAT_R8G8B8A8_UNORM};
	UINT sampleCount_ = 1;

	// render‑state ----------------------------------------------------//
	D3D12_RASTERIZER_DESC rasterizer_ {CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT)};
	D3D12_BLEND_DESC blend_ {CD3DX12_BLEND_DESC(D3D12_DEFAULT)};
	D3D12_DEPTH_STENCIL_DESC depth_ {CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT)};

public:
	//===================================================================*/
	//		operator/hash
	//===================================================================*/
	bool operator==(const GraphicsPipelineDesc& o) const noexcept;
	size_t Hash() const noexcept;
};

// ===============================================================//
//  mapなどのきーとして使うためのhash関数
//================================================================//
//	operatorとhashを定義することで
// std::unordered_mapなどのkeyとして使えるようにする
// 例:
// std::unordered_map<GraphicsPipelineDesc, ID3D12PipelineState*> pipelineCache;
// pipelineCache[desc] = pso;
//================================================================//
namespace std{
	template<>
	struct hash<GraphicsPipelineDesc>{
		size_t operator()(GraphicsPipelineDesc const& desc) const noexcept{
			return desc.Hash();
		}
	};
}