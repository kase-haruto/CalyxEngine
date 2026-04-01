#include "PsoFactory.h"

#include <Engine/Graphics/Context/GraphicsGroup.h>

#include <stdexcept>

std::unique_ptr<PipelineStateObject>
PsoFactory::Create(const GraphicsPipelineDesc& desc) {
	using Microsoft::WRL::ComPtr;
	if (!shaderCompiler_) { throw std::runtime_error("ShaderCompiler is null in PsoFactory"); }

	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	auto psoObj = std::make_unique<PipelineStateObject>();

	// RootSignature ----------------------------------------------------
	D3D12_ROOT_SIGNATURE_DESC rootDesc = desc.root_.Desc();

	ComPtr<ID3DBlob> sigBlob, errBlob;
	if (FAILED(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errBlob))) {
		if (errBlob) { OutputDebugStringA((char*)errBlob->GetBufferPointer()); }
		throw std::runtime_error("RootSignature serialization failed");
	}
	ComPtr<ID3D12RootSignature> rootSig;
	if (FAILED(device->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig)))) {
		throw std::runtime_error("CreateRootSignature failed");
	}
	psoObj->SetRootSignature(rootSig.Get());

	// ==== Compute PSO =================================================
	if (desc.isCompute_) {
		// CS をコンパイル
		if (desc.cs_.empty()) {
			throw std::runtime_error("Compute pipeline requires CS.");
		}
		auto csBlob = shaderCompiler_->CompileShader(L"Resources/shaders/" + desc.cs_, L"cs_6_0");

		D3D12_COMPUTE_PIPELINE_STATE_DESC cpso{};
		cpso.pRootSignature = rootSig.Get();
		cpso.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
		cpso.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

		if (!psoObj->Initialize(cpso)) {
			throw std::runtime_error("Compute PipelineState initialization failed");
		}

		// 既存の SetShaderBlobs(vs, ps) しかない場合は、ps側にcsを入れておく（簡易対応）
		psoObj->SetShaderBlobs(nullptr, csBlob.Get());
		return psoObj;
	}

	// ==== Graphics PSO ===============================================
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
	pso.pRootSignature = rootSig.Get();
	pso.InputLayout = { desc.inputElems_.data(), static_cast<UINT>(desc.inputElems_.size()) };
	pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso.SampleDesc.Count = desc.sampleCount_;
	pso.RasterizerState = desc.rasterizer_;
	pso.BlendState = desc.blend_;
	pso.DepthStencilState = desc.depth_;
	pso.SampleMask = UINT_MAX;
	pso.NumRenderTargets = static_cast<UINT>(desc.rtvFormats_.size());
	for (size_t i = 0; i < desc.rtvFormats_.size(); ++i) { pso.RTVFormats[i] = desc.rtvFormats_[i]; }
	pso.DSVFormat = desc.dsvFormat_;

	auto compile = [&](const std::wstring& path, const wchar_t* profile) -> ComPtr<IDxcBlob> {
		return shaderCompiler_->CompileShader(L"Resources/shaders/" + path, profile);
	};

	ComPtr<IDxcBlob> vsBlob, psBlob, gsBlob;
	if (!desc.vs_.empty()) {
		vsBlob = compile(desc.vs_, L"vs_6_5");
		pso.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
	}
	if (!desc.ps_.empty()) {
		psBlob = compile(desc.ps_, L"ps_6_5");
		pso.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };
	}
	if (!desc.gs_.empty()) {
		gsBlob = compile(desc.gs_, L"gs_6_5");
		pso.GS = { gsBlob->GetBufferPointer(), gsBlob->GetBufferSize() };
	}

	if (!psoObj->Initialize(pso)) {
		throw std::runtime_error("PipelineState initialization failed");
	}
	psoObj->SetShaderBlobs(vsBlob, psBlob);
	// Note: SetShaderBlobs might need to be updated if it needs to store GS blob too, 
	// but usually blobs are just for lifetime during Create. 
	// If PipelineStateObject keeps them, I should check its definition.
	return psoObj;
}