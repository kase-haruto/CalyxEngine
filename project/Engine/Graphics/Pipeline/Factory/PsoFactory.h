#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */

// engine
#include <Engine/Graphics/Pipeline/PipelineDesc/GraphicsPipelineDesc.h>
#include <Engine/Graphics/Pipeline/Pso/PipelineStateObject.h>
#include <Engine/Graphics/Pipeline/Shader/ShaderCompiler.h>

// c++

/* ========================================================================
/*		psoを作成
/* ===================================================================== */
class PsoFactory {
public:
	//===================================================================*/
	//		public functions
	//===================================================================*/
	PsoFactory() = default;
	PsoFactory(ShaderCompiler* compiler) : shaderCompiler_(compiler) {}
	~PsoFactory() = default;

	/// <summary>
	/// 作成
	/// </summary>
	/// <param name="desc"></param>
	/// <returns></returns>
	std::unique_ptr<PipelineStateObject> Create(const GraphicsPipelineDesc& desc);

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	ShaderCompiler* shaderCompiler_ = nullptr;
};

