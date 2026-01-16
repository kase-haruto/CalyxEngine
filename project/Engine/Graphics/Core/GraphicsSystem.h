#pragma once

// engine
#include <Engine/Graphics/Pipeline/Service/PipelineService.h>

// c++
#include <memory>

/*-----------------------------------------------------------------------------------------
 * GraphicsSystem
 * - グラフィックスシステム管理クラス
 * - パイプラインサービスやコマンドリストの管理を担当
 *---------------------------------------------------------------------------------------*/
class GraphicsSystem {
public:
	//===================================================================*/
	//		public methods
	//===================================================================*/
	GraphicsSystem() = default;
	~GraphicsSystem() = default;

	void Initialize();

	//--------- accessor --------------------------------------------------
	// Pipeline
	PipelineService* GetPipelineService()const;

	// dxObjects
	ID3D12GraphicsCommandList* GetCommandList() const;

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	std::unique_ptr<PipelineService> pipelineService_;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

};
