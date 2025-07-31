#include "GpuFxEmitter.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>

// ────────────────────────────────────────────────────────────────
//  ctor / dtor
// ────────────────────────────────────────────────────────────────
GpuFxEmitter::GpuFxEmitter() = default;
GpuFxEmitter::~GpuFxEmitter() = default;

// ────────────────────────────────────────────────────────────────
//  リソース生成
// ────────────────────────────────────────────────────────────────
void GpuFxEmitter::Initialize(){
	ID3D12Device* dev = GraphicsGroup::GetInstance()->GetDevice().Get();

	// StructuredBuffer を DEFAULT + UAV で確保
	particleBuffer_.InitializeAsRW(dev, kMaxParticles);
	particleBuffer_.CreateUav(dev);   // u0
	particleBuffer_.CreateSrv(dev);   // t0

	paramBuffer_.Initialize(dev);
}

// ────────────────────────────────────────────────────────────────
//  Update: 毎フレーム deltaTime を積む
// ────────────────────────────────────────────────────────────────
void GpuFxEmitter::Update(float dt){
	emitParam_.deltaTime = dt;
	// acceleration などを外部から変更したいならここで
}

// ────────────────────────────────────────────────────────────────
//  Dispatch: CS でパーティクル更新
// ────────────────────────────────────────────────────────────────
void GpuFxEmitter::Dispatch(ID3D12GraphicsCommandList* cmd){
	if (!cmd) return;

	auto* res = particleBuffer_.GetResource().Get();

	// 1) SRV → UAV
	CD3DX12_RESOURCE_BARRIER toUav =
		CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cmd->ResourceBarrier(1, &toUav);

	// 2) b0: 定数
	paramBuffer_.TransferData(emitParam_);
	paramBuffer_.SetCommand(cmd, 0);

	cmd->SetComputeRootDescriptorTable(1, particleBuffer_.GetGpuUavHandle());

	cmd->Dispatch(kMaxParticles, 1, 1);

	// 5) UAV → SRV (描画パス用)
	CD3DX12_RESOURCE_BARRIER toSrv =
		CD3DX12_RESOURCE_BARRIER::Transition(
		res,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	cmd->ResourceBarrier(1, &toSrv);
}