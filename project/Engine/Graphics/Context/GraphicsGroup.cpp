#include "GraphicsGroup.h"

GraphicsGroup* GraphicsGroup::GetInstance(){
	static GraphicsGroup instance;
	return& instance;
}

void GraphicsGroup::Initialize(const CalyxEngine::DxCore* dxCore, PipelineStateManager* psManager){
	pDxCore_ = dxCore;
	pipelineManager_ = psManager;
}

PipelineStateManager* GraphicsGroup::GetPipelineState()const{ return pipelineManager_; }

const ComPtr<ID3D12PipelineState>& GraphicsGroup::GetPipelineState(const PipelineType& type,const BlendMode& blendMode)const { return pipelineManager_->GetPipelineState(type,blendMode); }

const ComPtr<ID3D12RootSignature>& GraphicsGroup::GetRootSignature(const PipelineType& type, const BlendMode& blendMode)const{ return pipelineManager_->GetRootSignature(type,blendMode); }

ComPtr<ID3D12Device> GraphicsGroup::GetDevice() const{ return pDxCore_->GetDevice(); }
ID3D12Device5*       GraphicsGroup::GetDevice5() const {return pDxCore_->GetDevice5();}

ComPtr<ID3D12GraphicsCommandList> GraphicsGroup::GetCommandList() const{ return pDxCore_->GetCommandList(); }
uint32_t						  GraphicsGroup::GetClientWidth() const { return pDxCore_->GetClientWidth(); }
uint32_t						  GraphicsGroup::GetClientHeight() const { return pDxCore_->GetClientHeight(); }

void GraphicsGroup::SetCommand(ComPtr<ID3D12GraphicsCommandList> commandList, PipelineType psoType, BlendMode blendMode){
	commandList->SetPipelineState(pipelineManager_->GetPipelineState(psoType, blendMode).Get());
	commandList->SetGraphicsRootSignature(pipelineManager_->GetRootSignature(psoType, blendMode).Get());
}