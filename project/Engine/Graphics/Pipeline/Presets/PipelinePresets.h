#pragma once

#include <Engine/Graphics/Pipeline/PipelineDesc/GraphicsPipelineDesc.h>
#include <Engine/Graphics/Pipeline/RootSig/RootSignatureBuilder.h>

namespace PipelinePresets {

	// 3dObjects
	GraphicsPipelineDesc MakeObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeSkinningObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeParticle(BlendMode mode);
	GraphicsPipelineDesc MakeGpuParticle(BlendMode mode);

	GraphicsPipelineDesc MakeWireframeObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeWireframeSkinnedObject3D(BlendMode mode);
	// shadow
	GraphicsPipelineDesc MakeShadowStatic();
	GraphicsPipelineDesc MakeShadowSkinned();

	//デバッグピッキング用(マウスでオブジェクト選択
#if defined(_DEBUG) || defined(DEVELOP)
	// picking
	GraphicsPipelineDesc MakePickingStatic();
	GraphicsPipelineDesc MakePickingSkinned();
#endif

	// 2dObjects
	GraphicsPipelineDesc MakeObject2D();

	// cs
	GraphicsPipelineDesc MakeGpuParticleCS();
	GraphicsPipelineDesc MakeGpuParticleEmit();
	GraphicsPipelineDesc MakeGpuParticleUpdate();

	// postprocess
	GraphicsPipelineDesc MakeCopyImage();
	GraphicsPipelineDesc MakeGrayScale();
	GraphicsPipelineDesc MakeRadialBlur();
	GraphicsPipelineDesc MakeGrayScale();
	GraphicsPipelineDesc MakeChromaticAberration();
	GraphicsPipelineDesc MakeVignette();
	GraphicsPipelineDesc MakeCRT();
}