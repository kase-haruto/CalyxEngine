#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */

// engine
#include <Engine/Graphics/Pipeline/Factory/PsoFactory.h>
#include <Engine/Graphics/Pipeline/PipelineDesc/GraphicsPipelineDesc.h>
#include <Engine/Graphics/Pipeline/Pso/PipelineStateObject.h>

// c++
#include <unordered_map>

class PsoLibrary {
public:
	//===================================================================*/
	//		public functions
	//===================================================================*/
	PsoLibrary() = default;
	PsoLibrary(PsoFactory* factory) : factory_(factory) {}
	~PsoLibrary() = default;

	// 取得　==============================================================*/
	ID3D12PipelineState* GetOrCreate(const GraphicsPipelineDesc& desc);
	ID3D12RootSignature* GetRoot(const GraphicsPipelineDesc& desc);

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	std::unordered_map<GraphicsPipelineDesc, std::unique_ptr<PipelineStateObject>> psoCache_;
	PsoFactory* factory_{ nullptr };

};

