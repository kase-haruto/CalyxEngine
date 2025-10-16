#include "PipelinePresets.h"

#include <Engine/Graphics/Pipeline/PipelineDesc/Input/VertexLayout.h>

/* ================================================================================================
/*							Objects
/* ================================================================================================ */

/////////////////////////////////////////////////////////////////////////////////////////
//		object3D
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeObject3D(BlendMode mode) {
	GraphicsPipelineDesc desc;
	desc.VS(L"Object3d.VS.hlsl")
		.PS(L"Object3d.PS.hlsl")
		.Input(VertexInputLayout<VertexPosUvN>::Get())
		.Blend(mode)
		.CullNone()
		.DepthEnable(true)
		.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.root_
		.AllowIA()
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL)                                         // Material
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_VERTEX) // WVP
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)  // Tex
		.CBV(2,D3D12_SHADER_VISIBILITY_PIXEL)                                         // DirLight
		.CBV(1,D3D12_SHADER_VISIBILITY_ALL)                                           // Camera
		.CBV(4,D3D12_SHADER_VISIBILITY_PIXEL)                                         // PointLight
		.SRVTable(1,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)  // EnvMap
		.SRVTable(1,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_VERTEX) // billboard
		.SamplerWrapLinear(0);

	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		skinModel
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeSkinningObject3D(BlendMode mode) {
	GraphicsPipelineDesc desc;
	desc.VS(L"SkinningObject3d.VS.hlsl")
		.PS(L"Object3d.PS.hlsl")
		.Input(VertexInputLayout<VertexPosUvNSkinning>::Get())
		.Blend(mode)
		.CullBack()
		.DepthEnable(true)
		.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.root_
		.AllowIA()
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL)                                         // Material
		.CBV(0,D3D12_SHADER_VISIBILITY_VERTEX)                                        // WVP
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)  // Tex
		.CBV(2,D3D12_SHADER_VISIBILITY_PIXEL)                                         // DirLight
		.CBV(1,D3D12_SHADER_VISIBILITY_ALL)                                           // Camera
		.CBV(4,D3D12_SHADER_VISIBILITY_PIXEL)                                         // PointLight
		.SRVTable(1,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)  // EnvMap
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_VERTEX) // SkinningBuffer

		.SamplerWrapLinear(0);

	return desc;
}

////////////////////////////////////////////////////////////////////////////////////////
//		partcicle
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeParticle(BlendMode mode) {
	D3D12_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable              = TRUE;
	depthDesc.DepthWriteMask           = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みを無効にする
	depthDesc.DepthFunc                = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthDesc.StencilEnable            = FALSE;

	GraphicsPipelineDesc desc;
	desc.VS(L"Particle.VS.hlsl")
		.PS(L"Particle.PS.hlsl")
		.Input(VertexInputLayout<VertexPosUvN>::Get())
		.Blend(mode)
		.CullBack()
		.DepthState(depthDesc)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.root_
		.AllowIA()
		.CBV(0,D3D12_SHADER_VISIBILITY_VERTEX)                                        // gCamera (b0)
		.CBV(1,D3D12_SHADER_VISIBILITY_PIXEL)                                         // gMaterial (b1)
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_VERTEX) // gParticle (t0)
		.SRVTable(1,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)  // gTexture  (t1)
		.SamplerWrapLinear(0);                                                        // gSampler (s0)

	return desc;
}

GraphicsPipelineDesc PipelinePresets::MakeGpuParticle(BlendMode mode) {
	D3D12_DEPTH_STENCIL_DESC depthDesc = {};
	depthDesc.DepthEnable              = TRUE;
	depthDesc.DepthWriteMask           = D3D12_DEPTH_WRITE_MASK_ZERO; // 書き込みを無効にする
	depthDesc.DepthFunc                = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	depthDesc.StencilEnable            = FALSE;

	GraphicsPipelineDesc desc;
	desc.VS(L"GpuParticle.VS.hlsl")
		.PS(L"Particle.PS.hlsl")
		.Input(VertexInputLayout<VertexPosUvN>::Get())
		.Blend(mode)
		.CullNone()
		.DepthState(depthDesc)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.root_
		.AllowIA()
		.CBV(0,D3D12_SHADER_VISIBILITY_VERTEX)                                        // gCamera (b0)
		.CBV(1,D3D12_SHADER_VISIBILITY_PIXEL)                                         // gMaterial (b1)
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_VERTEX) // gParticle (t0)
		.SRVTable(1,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)  // gTexture  (t1)
		.SamplerWrapLinear(0);                                                        // gSampler (s0)

	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		2dObject
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeObject2D() {
	GraphicsPipelineDesc desc;

	// 頂点シェーダ・ピクセルシェーダ指定
	desc.VS(L"Object2d.VS.hlsl")
		.PS(L"Object2d.PS.hlsl")

		// 入力レイアウトは VertexPosUv4 （Vector4 pos + Vector2 uv）用
		.Input(VertexInputLayout<VertexData>::Get())

		// アルファブレンド
		.Blend(BlendMode::ALPHA)

		// カリングなし
		.CullNone()

		// 深度無効
		.DepthEnable(false)
		.DepthFunc(D3D12_COMPARISON_FUNC_ALWAYS)

		// レンダーターゲット
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)

		// サンプル数
		.Samples(1);

	// ルートシグネチャ設定
	desc.root_
		.AllowIA()
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL)
		.CBV(0,D3D12_SHADER_VISIBILITY_VERTEX)
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.SamplerWrapLinear(0);

	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		gpuParticle
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeGpuParticleCS() {
	GraphicsPipelineDesc desc;
	desc.CS(L"InitializeParticle.CS.hlsl");

	desc.root_
		// b0: EmitterParams（deltaTime, acceleration）
		.CBV(0,D3D12_SHADER_VISIBILITY_ALL)
		.UAVTable(0,1)  // u0 : RWStructuredBuffer<Particle>
		.UAVTable(1,1)  // u0 : RWStructuredBuffer<Particle>
		.UAVTable(2,1); // u1 : RWStructuredBuffer<uint> (freeCounter)

	return desc;
}

GraphicsPipelineDesc PipelinePresets::MakeGpuParticleEmit() {
	GraphicsPipelineDesc desc;
	desc.CS(L"EmitParticle.CS.hlsl");

	desc.root_
		// b0: EmitterParams
		.CBV(0,D3D12_SHADER_VISIBILITY_ALL)
		.CBV(1,D3D12_SHADER_VISIBILITY_ALL)
		// u0: RWStructuredBuffer<Particle>
		.UAVTable(0,1)  // u0 : RWStructuredBuffer<Particle>
		.UAVTable(1,1)  // u1 : RWStructuredBuffer<uint> (freeListIndex)
		.UAVTable(2,1); // u2 : RWStructuredBuffer<uint> (freeList)

	return desc;
}

GraphicsPipelineDesc PipelinePresets::MakeGpuParticleUpdate() {
	GraphicsPipelineDesc desc;
	desc.CS(L"UpdateParticle.CS.hlsl");

	desc.root_
		.CBV(0,D3D12_SHADER_VISIBILITY_ALL) // b0 frameTime
		.UAVTable(0,1)         // u0 : RWStructuredBuffer<Particle>
		.UAVTable(1,1)         // u1 : RWStructuredBuffer<uint> (freeListIndex)
		.UAVTable(2,1);        // u2 : RWStructuredBuffer<uint> (freeList)

	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		primitiveObject
/////////////////////////////////////////////////////////////////////////////////////////


/* ================================================================================================
/*							postProcess
/* ================================================================================================ */

/////////////////////////////////////////////////////////////////////////////////////////
//		CopyImage
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeCopyImage() {
	GraphicsPipelineDesc desc;
	desc.VS(L"CopyImage.VS.hlsl")
		.PS(L"CopyImage.PS.hlsl")
		.BlendNone()
		.CullNone()
		.DepthEnable(false)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.inputElems_.clear();

	desc.root_
		.AllowIA()
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.SampleClampLinear(0);
	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		GrayScale
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeGrayScale() {
	GraphicsPipelineDesc desc;
	desc.VS(L"CopyImage.VS.hlsl")
		.PS(L"GrayScale.PS.hlsl")
		.BlendNone()
		.CullNone()
		.DepthEnable(false)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.inputElems_.clear();

	desc.root_
		.AllowIA()
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.SampleClampLinear(0);

	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ChormaticAberration
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeChromaticAberration() {
	GraphicsPipelineDesc desc;
	desc.VS(L"CopyImage.VS.hlsl")
		.PS(L"ChromaticAberration.PS.hlsl")
		.BlendNone()
		.CullNone()
		.DepthEnable(false)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.inputElems_.clear();

	desc.root_
		.AllowIA()
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL) // Distortion parameters
		.SampleClampLinear(0);

	return desc;
}


/////////////////////////////////////////////////////////////////////////////////////////
//		vignette
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeVignette() {
	GraphicsPipelineDesc desc;
	desc.VS(L"CopyImage.VS.hlsl")
		.PS(L"Vignette.PS.hlsl")
		.BlendNone()
		.CullNone()
		.DepthEnable(false)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.inputElems_.clear();

	desc.root_
		.AllowIA()
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL)
		.SampleClampLinear(0);
	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		CRT
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeCRT() {
	GraphicsPipelineDesc desc;
	desc.VS(L"CopyImage.VS.hlsl")
		.PS(L"CRT.PS.hlsl")
		.BlendNone()
		.CullNone()
		.DepthEnable(false)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.inputElems_.clear();

	desc.root_
		.AllowIA()
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL) // crt parameters
		.SampleClampLinear(0);
	return desc;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		radialBlur
/////////////////////////////////////////////////////////////////////////////////////////
GraphicsPipelineDesc PipelinePresets::MakeRadialBlur() {
	GraphicsPipelineDesc desc;
	desc.VS(L"CopyImage.VS.hlsl")
		.PS(L"RadialBlur.PS.hlsl")
		.BlendNone()
		.CullNone()
		.DepthEnable(false)
		.RTV(DXGI_FORMAT_R8G8B8A8_UNORM)
		.Samples(1);

	desc.inputElems_.clear();

	desc.root_
		.AllowIA()
		.SRVTable(0,1,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,D3D12_SHADER_VISIBILITY_PIXEL)
		.CBV(0,D3D12_SHADER_VISIBILITY_PIXEL) // Blur parameters
		.SampleClampLinear(0);
	return desc;
}