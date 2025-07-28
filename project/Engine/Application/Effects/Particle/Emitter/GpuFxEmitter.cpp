#include "GpuFxEmitter.h"
#include <Engine/Graphics/Context/GraphicsGroup.h>

/////////////////////////////////////////////////////////////////////////////////////////
//		コンストラクタ
/////////////////////////////////////////////////////////////////////////////////////////
GpuFxEmitter::GpuFxEmitter() {
	ID3D12Device* device = GraphicsGroup::GetInstance()->GetDevice().Get();
	// マテリアルの初期化
	material_.color = Vector4(1, 1, 1, 1);
	materialBuffer_.Initialize(device);

	instanceBuffer_.Initialize(device, kMaxParticles);
	instanceBuffer_.CreateSrv(device);

	paramBuffer_.Initialize(device);

	particleBuffer_.Initialize(device, kMaxParticles);
	particleBuffer_.CreateUav(device);

	velocity_ = FxParam<Vector3>::MakeRandom(
		Vector3(-1.0f, 0.0f, -1.0f),
		Vector3(1.0f, 0.0f, 1.0f)
	);

	lifetime_ = FxParam<float>::MakeRandom(1.0f, 3.0f);
	scale_ = FxParam<Vector3>::MakeConstant();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		解放
/////////////////////////////////////////////////////////////////////////////////////////
GpuFxEmitter::~GpuFxEmitter() {
	instanceBuffer_.ReleaseSrv();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		更新
/////////////////////////////////////////////////////////////////////////////////////////
void GpuFxEmitter::Update(float deltaTime){
	emitParm_.deltaTime = deltaTime;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ディスパッチ
/////////////////////////////////////////////////////////////////////////////////////////
void GpuFxEmitter::Dispatch(ID3D12GraphicsCommandList* cmdList) {
	if (!cmdList) return;

	// 定数バッファ転送
	paramBuffer_.TransferData(emitParm_);
	paramBuffer_.SetCommand(cmdList, 0); // b0

	// UAV（構造化バッファ）
	D3D12_GPU_VIRTUAL_ADDRESS uavAddress = particleBuffer_.GetResource()->GetGPUVirtualAddress();
	cmdList->SetComputeRootUnorderedAccessView(1, uavAddress); // u0

	// Dispatch 実行
	cmdList->Dispatch((kMaxParticles + 255) / 256, 1, 1);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		送信
/////////////////////////////////////////////////////////////////////////////////////////
void GpuFxEmitter::TransferParticleDataToGPU() {
	BaseEmitter::TransferParticleDataToGPU();

	std::vector<ParticleCS> particles(kMaxParticles);
	for (auto& p : particles) {
		p.translate = position_;
		p.velocity = velocity_.Get();
		p.scale = scale_.Get();
		p.lifeTime = lifetime_.Get();
		p.currentTIme = 0.0f;
		p.color = Vector4(1, 1, 1, 1);
	}

	particleBuffer_.TransferVectorData(particles);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		座標のセット
/////////////////////////////////////////////////////////////////////////////////////////
void GpuFxEmitter::SetPosition(const Vector3& pos){
	position_ = pos;
}
