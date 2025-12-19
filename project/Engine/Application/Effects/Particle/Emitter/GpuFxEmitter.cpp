#include "GpuFxEmitter.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>

namespace CalyxEffect {
	// ────────────────────────────────────────────────────────────────
	//  ctor / dtor
	// ────────────────────────────────────────────────────────────────
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
		paramBuffer_.TransferData(emitParam_);

		material_.color = CalyxMath::Vector4(1, 1, 1, 1);
		materialBuffer_.Initialize(dev);

		instanceBuffer_.Initialize(dev, kMaxParticles);
		instanceBuffer_.CreateSrv(dev);

		emitterData_.translate = {0, 0, 0};
		emitterData_.radius = 6.0f;
		emitterData_.count = 1000000;
		emitterData_.frequency = 0.0f;
		emitterData_.frequencyTime = 0.0f;
		emitterData_.emit = 1;

		emitterParamBuf_.Initialize(dev);
		perFrameBuffer_.Initialize(dev);

		freeListIndexBuffer_.InitializeAsRW(dev, 1);        // u1
		freeListIndexBuffer_.CreateUav(dev);

		freeListBuffer_.InitializeAsRW(dev, kMaxParticles); // u2
		freeListBuffer_.CreateUav(dev);
	}

	// ────────────────────────────────────────────────────────────────
	//  Update: 毎フレーム deltaTime を積む
	// ────────────────────────────────────────────────────────────────
	void GpuFxEmitter::Update(float dt){
		emitParam_.deltaTime = dt;
		perFrame_.deltaTime = dt;
		perFrame_.time += dt;
		emitterData_.frequencyTime += dt;

		if (emitterData_.frequency <= emitterData_.frequencyTime){
			emitterData_.frequencyTime -= emitterData_.frequency;
			emitterData_.emit = 1;
		} else{
			emitterData_.emit = 0;
		}

		if (perFrame_.time >= 50) {
			perFrame_.time = 0;
		}

		perFrameBuffer_.TransferData(perFrame_);
		emitterParamBuf_.TransferData(emitterData_);
		materialBuffer_.TransferData(material_);
	}

	void GpuFxEmitter::ShowGui() {
		ImGui::Begin("GPU FX Emitter");

		if(ImGui::CollapsingHeader("Emitter Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::DragFloat3("Position", &emitterData_.translate.x, 0.1f);
			ImGui::DragFloat("Radius", &emitterData_.radius, 0.01f, 0.0f, 100.0f);
			ImGui::DragInt("Emit Count", reinterpret_cast<int*>(&emitterData_.count), 1, 0, kMaxParticles);
			ImGui::DragFloat("Frequency", &emitterData_.frequency, 0.01f, 0.01f, 100.0f);
			ImGui::Checkbox("Emit Active", reinterpret_cast<bool*>(&emitterData_.emit));
		}

		ImGui::End();
	}
	void GpuFxEmitter::ApplyConfigFrom(const EmitterConfig& config) {
		(void)config;
	}
	void GpuFxEmitter::ExtractConfigTo(EmitterConfig& config) const {
		(void)config;
	}

	// ────────────────────────────────────────────────────────────────
	//  Dispatch: CS でパーティクル更新
	// ────────────────────────────────────────────────────────────────
	void GpuFxEmitter::DispatchInitialize(ID3D12GraphicsCommandList* cmd){
		if (!cmd || isInitialized) return;

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
		paramBuffer_.SetCompute(cmd, 0);
		cmd->SetComputeRootDescriptorTable(1, particleBuffer_.GetGpuUavHandle());
		cmd->SetComputeRootDescriptorTable(2, freeListIndexBuffer_.GetGpuUavHandle()); // u1: Counter
		cmd->SetComputeRootDescriptorTable(3, freeListBuffer_.GetGpuUavHandle()); // u1: Counter

		cmd->Dispatch(1024, 1, 1);

		// 5) UAV → SRV (描画パス用)
		CD3DX12_RESOURCE_BARRIER toSrv =
			CD3DX12_RESOURCE_BARRIER::Transition(
			res,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		cmd->ResourceBarrier(1, &toSrv);

		isInitialized = true;
	}

	void GpuFxEmitter::DispatchEmit(ID3D12GraphicsCommandList* cmd){
		if (!cmd) return;

		auto* res = particleBuffer_.GetResource().Get();
		auto* counterRes = freeListIndexBuffer_.GetResource().Get();
		auto* listRes = freeListBuffer_.GetResource().Get();

		// ── SRV → UAV 遷移 ───────────────────────────────
		CD3DX12_RESOURCE_BARRIER toUav[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		};
		cmd->ResourceBarrier(_countof(toUav), toUav);

		// ── Root 設定 ────────────────────────────────────
		emitterParamBuf_.SetCompute(cmd, 0);       // b0: Emitter
		perFrameBuffer_.SetCompute(cmd, 1);        // b1: PerFrame
		cmd->SetComputeRootDescriptorTable(2, particleBuffer_.GetGpuUavHandle());   // u0: Particles
		cmd->SetComputeRootDescriptorTable(3, freeListIndexBuffer_.GetGpuUavHandle()); // u1: listIndex
		cmd->SetComputeRootDescriptorTable(4, freeListBuffer_.GetGpuUavHandle()); // u2: list

		// ── Dispatch（1スレッドでEmit）────────────────────
		cmd->Dispatch(1, 1, 1);

		// ── UAV → SRV に戻す ─────────────────────────────
		CD3DX12_RESOURCE_BARRIER toSrv[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		};
		cmd->ResourceBarrier(_countof(toSrv), toSrv);
	}

	void GpuFxEmitter::DispatchUpdate(ID3D12GraphicsCommandList* cmd){
		if (!cmd) return;

		auto* res = particleBuffer_.GetResource().Get();
		auto* counterRes = freeListIndexBuffer_.GetResource().Get();
		auto* listRes = freeListBuffer_.GetResource().Get();

		// ── SRV → UAV 遷移 ───────────────────────────────
		CD3DX12_RESOURCE_BARRIER toUav[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		};

		cmd->ResourceBarrier(_countof(toUav), toUav);

		// ── Root 設定 ────────────────────────────────────
		perFrameBuffer_.SetCompute(cmd, 0);        // b0: PerFrame
		cmd->SetComputeRootDescriptorTable(1, particleBuffer_.GetGpuUavHandle());		// u0: Particles
		cmd->SetComputeRootDescriptorTable(2, freeListIndexBuffer_.GetGpuUavHandle());	// u1: listIndex
		cmd->SetComputeRootDescriptorTable(3, freeListBuffer_.GetGpuUavHandle());		// u2: list

		// ── Dispatch ────────────────────
		cmd->Dispatch(1024, 1, 1);

		// ── UAV → SRV に戻す ─────────────────────────────
		CD3DX12_RESOURCE_BARRIER toSrv[] = {
			CD3DX12_RESOURCE_BARRIER::Transition(
				res,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				counterRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),

			CD3DX12_RESOURCE_BARRIER::Transition(
				listRes,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
				D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		};
		cmd->ResourceBarrier(_countof(toSrv), toSrv);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GpuFxEmitter::GetParticleSrv() const {
		return particleBuffer_.GetGpuSrvHandle();
	}
}