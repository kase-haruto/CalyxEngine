#include "PipelineStateObject.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>
#include <Engine/Foundation/Log/EngineLogger.h>

void PipelineStateObject::SetRootSignature(ID3D12RootSignature* root) {
	// Root Signatureの所有権はLibrary側に残し、PSOと併用する非所有参照だけを保持する。
	rootSignature_ = root;
}

bool PipelineStateObject::Initialize(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) {
	// GraphicsGroupが所有するDeviceから描画用PSOを生成し、ComPtrでLifetimeを管理する。
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	HRESULT hr = device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		// HRESULTをDebuggerとEngine Logの両方へ残し、Editor外の起動失敗も診断可能にする。
		char buf[128];
		sprintf_s(buf, "CreateGraphicsPipelineState failed. hr=0x%08X\n", (unsigned)hr);
		OutputDebugStringA(buf);
		CalyxEngine::EngineLogger::GetInstance().Add(
			CalyxEngine::LogLevel::Error,
			CalyxEngine::LogCategory::Rendering,
			"Failed to create graphics pipeline state. HRESULT=" + std::to_string(static_cast<unsigned>(hr)),
			"PipelineStateObject");
		return false;
	}
	return true;
}

bool PipelineStateObject::Initialize(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) {
	// Compute Descriptorから同じPSO保持領域へ生成し、種別FlagでBinding経路を識別する。
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	HRESULT hr = device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		OutputDebugStringA("Failed to create compute pipeline state\n");
		CalyxEngine::EngineLogger::GetInstance().Add(
			CalyxEngine::LogLevel::Error,
			CalyxEngine::LogCategory::Rendering,
			"Failed to create compute pipeline state. HRESULT=" + std::to_string(static_cast<unsigned>(hr)),
			"PipelineStateObject");
		return false;
	}
	// 生成成功後だけCompute扱いへ切り替え、失敗PSOをCommandListへ設定させない。
	isCompute_ = true;
	return true;
}
