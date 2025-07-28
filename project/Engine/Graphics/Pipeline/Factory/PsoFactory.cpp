#include "PsoFactory.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <stdexcept>

std::unique_ptr<PipelineStateObject>
PsoFactory::Create(const GraphicsPipelineDesc& desc) {
	using Microsoft::WRL::ComPtr;
	if (!shaderCompiler_)
		throw std::runtime_error("ShaderCompiler is null in PsoFactory");

	ID3D12Device* device =
		GraphicsGroup::GetInstance()->GetDevice().Get();
	auto psoObj = std::make_unique<PipelineStateObject>();

	// ─────────────────────────────────────
	//  Root Signature
	// ─────────────────────────────────────
	D3D12_ROOT_SIGNATURE_DESC rootDesc = desc.root_.Desc();

	ComPtr<ID3DBlob> sigBlob, errBlob;
	if (FAILED(D3D12SerializeRootSignature(
		&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		&sigBlob, &errBlob))) {
		if (errBlob) OutputDebugStringA(
			(char*)errBlob->GetBufferPointer());
		throw std::runtime_error("RootSignature serialization failed");
	}
	ComPtr<ID3D12RootSignature> rootSig;
	if (FAILED(device->CreateRootSignature(
		0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSig))))
		throw std::runtime_error("CreateRootSignature failed");

	// 生成した RootSig を PSO オブジェクトにセット
	psoObj->SetRootSignature(rootSig.Get());

	// ─────────────────────────────────────
	//  Input Layout
	// ─────────────────────────────────────
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
	pso.pRootSignature = rootSig.Get();
	pso.InputLayout = { desc.inputElems_.data(),
						static_cast<UINT>(desc.inputElems_.size()) };

	// その他固定値
	pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso.SampleDesc.Count = desc.sampleCount_;
	pso.RasterizerState = desc.rasterizer_;
	pso.BlendState = desc.blend_;
	pso.DepthStencilState = desc.depth_;
	pso.SampleMask = UINT_MAX;
	pso.NumRenderTargets = static_cast<UINT>(desc.rtvFormats_.size());
	for (size_t i = 0; i < desc.rtvFormats_.size(); ++i)
		pso.RTVFormats[i] = desc.rtvFormats_[i];
	pso.DSVFormat = desc.dsvFormat_;

	// ─────────────────────────────────────
	//  Compile Shaders
	// ─────────────────────────────────────
	ComPtr<IDxcBlob> vsBlob;
	ComPtr<IDxcBlob> psBlob;

	auto compile = [&](const std::wstring& path,
					   const wchar_t* profile) -> ComPtr<IDxcBlob> {
		return shaderCompiler_->CompileShader(L"Resources/shaders/" + path, profile);
	};

	if (!desc.vs_.empty()) {
		vsBlob = compile(desc.vs_, L"vs_6_0");
		pso.VS = { vsBlob->GetBufferPointer(),
				   vsBlob->GetBufferSize() };
	}
	if (!desc.ps_.empty()) {
		psBlob = compile(desc.ps_, L"ps_6_0");
		pso.PS = { psBlob->GetBufferPointer(),
				   psBlob->GetBufferSize() };
	}

	// PSO 作成
	if (!psoObj->Initialize(pso))
		throw std::runtime_error("PipelineState initialization failed");

	// BLOB を保持（Hot-Reload 用）
	psoObj->SetShaderBlobs(vsBlob, psBlob);

	return psoObj;
}