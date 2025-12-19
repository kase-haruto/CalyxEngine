#pragma once
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Engine/Application/Effects/Particle/Detail/ParticleDetail.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Material.h>

namespace CalyxEffect {
	/* ========================================================================
	/*	particle emitter
	/* ===================================================================== */
	class BaseEmitter {
	public:
		//===================================================================*/
		//					public func
		//===================================================================*/
		CalyxEffect::BaseEmitter();
		virtual ~BaseEmitter() = default;

		virtual void Update(float deltaTime) = 0;
		virtual void TransferParticleDataToGPU();

		/**
		 * \brief 再生
		 */
		virtual void Play() {}
		/**
		 *  \brief 停止
		 */
		virtual void Stop() {}
		/**
		 * \brief 再生中か
		 * \return true:再生中 false:停止中
		 */
		virtual bool IsPlaying() const { return true; }
		// 適用
		virtual void ApplyConfigFrom(const EmitterConfig& config) = 0;
		// 掃き出し
		virtual void ExtractConfigTo(EmitterConfig& config) const = 0;

	public:
		virtual CalyxMath::Vector3									 GetWorldPosition() const { return position_; }
		const std::string&											 GetTexturePath() const { return material_.texturePath; }
		const ParticleMaterial&										 GetMaterial() const { return material_; }
		const DxConstantBuffer<ParticleMaterial>&					 GetMaterialBuffer() const { return materialBuffer_; }
		const DxStructuredBuffer<ParticleConstantData>& GetInstanceBuffer() const { return instanceBuffer_; }
		const std::string&											 GetModelPath() const { return modelPath; }

	protected:
		std::string											  modelPath = "plane.obj"; //< モデルパス（デフォルトは平面
		CalyxMath::Vector3									  position_;			   //< emitterの位置
		ParticleMaterial									  material_;			   //< パーティクルのマテリアル
		std::vector<FxUnit>									  units_;				   //< パーティクルユニットの配列
		DxStructuredBuffer<ParticleConstantData> instanceBuffer_;
		DxConstantBuffer<ParticleMaterial>					  materialBuffer_; // パーティクルマテリアルの定数バッファ
	};

} // namespace CalyxEffect
