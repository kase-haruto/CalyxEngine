#pragma once
#include <Engine/Application/Effects/Particle/Detail/ParticleDetail.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>

#include <d3d12.h>

/* ========================================================================
/*	particle emitter
/* ===================================================================== */
class BaseEmitter {
public:
	//===================================================================*/
	//					public func
	//===================================================================*/
	virtual ~BaseEmitter() = default;

	virtual void Update(float deltaTime) = 0;
	virtual void TransferParticleDataToGPU();

	virtual void Play() {}
	virtual void Stop() {}
	virtual bool IsPlaying() const { return true; }

public:
	const std::string& GetTexturePath() const { return material_.texturePath; }
	const ParticleMaterial& GetMaterial() const { return material_; }
	const DxConstantBuffer<ParticleMaterial>& GetMaterialBuffer() const { return materialBuffer_; }
	const DxStructuredBuffer<ParticleConstantData>& GetInstanceBuffer() const { return instanceBuffer_; }

protected:
	ParticleMaterial material_;				//< パーティクルのマテリアル
	std::vector<FxUnit> units_;				//< パーティクルユニットの配列
	DxStructuredBuffer<ParticleConstantData> instanceBuffer_;
	DxConstantBuffer<ParticleMaterial> materialBuffer_; // パーティクルマテリアルの定数バッファ
};

