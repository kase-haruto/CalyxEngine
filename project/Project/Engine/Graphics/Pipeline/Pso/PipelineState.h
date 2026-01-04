#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
// engine
#include <Engine/Graphics/Pipeline/Shader/ShaderManager.h>
#include <Engine/Graphics/Pipeline/BlendMode/BlendMode.h>
#include <Engine/Graphics/Pipeline/Pso/PsoDetails.h>

// lib
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <dxcapi.h>
#include <memory>


class PipelineState{
public:
	PipelineState(Microsoft::WRL::ComPtr<ID3D12Device>device, std::shared_ptr<ShaderManager> shaderManager);
	~PipelineState();

	bool Initialize(const std::wstring& vsPath, const std::wstring& psPath,
					const D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
					const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc,
					const BlendMode& blendMode);

	PipelineSet GetPipelineSet() const{
		return {pipelineState_.Get(), rootSignature_.Get()};
	}

	const Microsoft::WRL::ComPtr<ID3D12PipelineState>& GetPipelineState()const{ return pipelineState_; }
	const Microsoft::WRL::ComPtr<ID3D12RootSignature>& GetRootSignature()const{ return rootSignature_; }

private:
	/// <summary>
	/// グラフィック関連
	/// </summary>
	Microsoft::WRL::ComPtr<ID3D12Device>device_;
	Microsoft::WRL::ComPtr<ID3D12RootSignature>rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>pipelineState_;

	/// <summary>
	/// shader
	/// </summary>
	Microsoft::WRL::ComPtr<IDxcBlob>vertexShader_;
	Microsoft::WRL::ComPtr<IDxcBlob>pixelShader_;
	std::shared_ptr<ShaderManager>shaderManager_;
};

