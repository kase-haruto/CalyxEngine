#include "GraphicsSystem.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Renderer/Text/TextService.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		graphics関連初期化
/////////////////////////////////////////////////////////////////////////////////////////
void GraphicsSystem::Initialize() {
	//===================================================================*/
	//		pipelines
	//===================================================================*/
	pipelineService_ = std::make_unique<PipelineService>();
	pipelineService_->RegisterAllPipelines();
	CalyxEngine::TextService::GetInstance()->Initialize(GraphicsGroup::GetInstance()->GetDevice().Get());

	//いったんコマンドリストをもらってくる
	commandList_ = GraphicsGroup::GetInstance()->GetCommandList();

}

/////////////////////////////////////////////////////////////////////////////////////////
//		パイプラインサービスの取得
/////////////////////////////////////////////////////////////////////////////////////////
PipelineService* GraphicsSystem::GetPipelineService() const {return pipelineService_.get();}

/////////////////////////////////////////////////////////////////////////////////////////
//		コマンドリスト取得
/////////////////////////////////////////////////////////////////////////////////////////
ID3D12GraphicsCommandList* GraphicsSystem::GetCommandList() const {	return commandList_.Get();}
