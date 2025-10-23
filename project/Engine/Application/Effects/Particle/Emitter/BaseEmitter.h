#pragma once
#include <Engine/Application/Effects/Particle/Detail/ParticleDetail.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Material.h>

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

	// でーた転送
	virtual void TransferParticleDataToGPU();

	virtual void Play() {} //< 再生
	virtual void Stop() {} //< 停止

	virtual bool IsPlaying() const { return true; }

	// getter
	virtual Vector3			GetWorldPosition() const { return position_; }
	const std::string&		GetTexturePath() const { return material_.texturePath; }
	const ParticleMaterial& GetMaterial() const { return material_; }
	const std::string&		GetModelPath() const { return modelPath; }

	const DxConstantBuffer<ParticleMaterial>&		GetMaterialBuffer() const { return materialBuffer_; }
	const DxStructuredBuffer<ParticleConstantData>& GetInstanceBuffer() const { return instanceBuffer_; }

protected:
	std::string			modelPath = "plane.obj"; //< モデルパス（デフォルトは平面
	Vector3				position_;				 //< emitterの位置
	ParticleMaterial	material_;				 //< パーティクルのマテリアル
	std::vector<FxUnit> units_;					 //< パーティクルユニットの配列

	// buff
	DxStructuredBuffer<ParticleConstantData> instanceBuffer_;
	DxConstantBuffer<ParticleMaterial>		 materialBuffer_; // パーティクルマテリアルの定数バッファ
};
