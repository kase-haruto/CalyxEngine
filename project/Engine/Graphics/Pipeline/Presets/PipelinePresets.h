#pragma once

#include <Engine/Graphics/Pipeline/PipelineDesc/GraphicsPipelineDesc.h>
#include <Engine/Graphics/Pipeline/RootSig/RootSignatureBuilder.h>

namespace PipelinePresets {

	// 3dObjects
	GraphicsPipelineDesc MakeObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeSkinningObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeParticle(BlendMode mode);
	GraphicsPipelineDesc MakeGpuParticle(BlendMode mode);
	GraphicsPipelineDesc MakeTrail(BlendMode mode);
	GraphicsPipelineDesc MakeEditorInfiniteGrid();

	GraphicsPipelineDesc MakeWireframeObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeWireframeSkinnedObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeOutlineObject3D();
	GraphicsPipelineDesc MakeOutlineSkinnedObject3D();
	GraphicsPipelineDesc MakeOutlineNormalObject3D();
	GraphicsPipelineDesc MakeOutlineNormalSkinnedObject3D();
	GraphicsPipelineDesc MakeOutlineDitherDepthObject3D();
	GraphicsPipelineDesc MakeOutlineDitherDepthSkinnedObject3D();
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
	GraphicsPipelineDesc MakeSkinningCompute();

	// postprocess
	GraphicsPipelineDesc MakeCopyImage();
	GraphicsPipelineDesc MakeGrayScale();
	GraphicsPipelineDesc MakeRadialBlur();
	GraphicsPipelineDesc MakeGrayScale();
	GraphicsPipelineDesc MakeChromaticAberration();
	GraphicsPipelineDesc MakeVignette();
	GraphicsPipelineDesc MakeCRT();
	GraphicsPipelineDesc MakeOutlineComposite();
	GraphicsPipelineDesc MakeBlend();
	GraphicsPipelineDesc MakeBloom();
	GraphicsPipelineDesc MakeDepthVisualize();
}
