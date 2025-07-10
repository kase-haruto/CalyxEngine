#pragma once

#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Detail/ParticleDetail.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

class ParticleRenderer{
public:
	//===================================================================*/
	//                    public functions
	//===================================================================*/
	void Render(const std::vector<std::shared_ptr<class FxEmitter>>& emitters,
				class PipelineService* pipelineService,
				ID3D12GraphicsCommandList* cmdList);

	void RenderGrouped(const std::string& modelPath,
					   const std::vector<ParticleConstantData>& gpuUnits,
					   ID3D12GraphicsCommandList* cmdList);

private:
	//===================================================================*/
	//                    private functions
	//===================================================================*/
	void EnsureModelIsReady(struct ModelData& model, ID3D12Device* device);
	void DrawModelInstanced(struct ModelData& model,
							ID3D12GraphicsCommandList* cmdList,
							UINT instanceCount,
							D3D12_GPU_DESCRIPTOR_HANDLE handle);

private:
	//===================================================================*/
	//                    private variables
	//===================================================================*/
	DxStructuredBuffer<ParticleConstantData> instanceBuffer_;
};
