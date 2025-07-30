#pragma once

#include <Engine/Graphics/Pipeline/PipelineDesc/GraphicsPipelineDesc.h>
#include <Engine/Graphics/Pipeline/RootSig/RootSignatureBuilder.h>

namespace PipelinePresets {

	// 3dObjects
	GraphicsPipelineDesc MakeObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeSkinningObject3D(BlendMode mode);
	GraphicsPipelineDesc MakeParticle(BlendMode mode);

	// 2dObjects
	GraphicsPipelineDesc MakeObject2D();

	// cs
	GraphicsPipelineDesc MakeGpuParticleCS();

	// postprocess
	GraphicsPipelineDesc MakeCopyImage();
	GraphicsPipelineDesc MakeGrayScale();
	GraphicsPipelineDesc MakeRadialBlur();
	GraphicsPipelineDesc MakeGrayScale();
	GraphicsPipelineDesc MakeChromaticAberration();
	GraphicsPipelineDesc MakeVignette();
	GraphicsPipelineDesc MakeCRT();
}
