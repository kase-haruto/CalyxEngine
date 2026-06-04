#pragma once
#include "Engine/Assets/Model/ModelData.h"

#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Engine/Application/Effects/Particle/Detail/ParticleDetail.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Material.h>
#include "EmitterDetails.h"
#include <Engine/Foundation/Math/Quaternion.h>

class WorldTransform;

namespace CalyxEngine {

	enum class ParticlePrimitives {
		plane = 0, //< 平面
		sphere,	   //< 球
		cube,	   //< 立方体
		cylinder,  //< 円柱
		cone,      //< 円錐
		torus,	   //< トーラス
		triangle,  //< 三角形
	};



	/*-----------------------------------------------------------------------------------------
	 * BaseEmitter
	 * - パーティクルエミッタ基底クラス
	 * - パーティクルの生成・更新・GPU転送の共通インターフェースを定義
	 *---------------------------------------------------------------------------------------*/
	class BaseEmitter {
	public:
		//===================================================================*/
		//					public func
		//===================================================================*/
		CalyxEngine::BaseEmitter();
		virtual ~BaseEmitter() = default;

		virtual void Update(float deltaTime) = 0;
		virtual void TransferParticleDataToGPU();
		virtual void ShowGui() {}
		/**
		 * \brief モデルデータのチェックと読み込み
		 */
		MeshResource& GetMeshResource();

		/**
		 * \brief 形状ごとに発生座標を生成
		 * \return 発生座標
		 */
		CalyxEngine::Vector3 GenerateSpawnPosition();
		CalyxEngine::Vector3 GenerateSpawnPosition(const CalyxEngine::Vector3& basePos);

		/**
		 * \brief 再生
		 */
		virtual void Play() {}
		/**
		 *  \brief 停止
		 */
		virtual void Stop() {}
		virtual void Reset() {}
		/**
		 * \brief 再生中か
		 * \return true:再生中 false:停止中
		 */
		virtual bool IsPlaying() const { return true; }
		// 適用
		virtual void ApplyConfigFrom(const EmitterConfig& config) = 0;
		// 掃き出し
		virtual void ExtractConfigTo(EmitterConfig& config) const = 0;

		virtual void SetAlphaMultiplier(float a) { alphaMultiplier_ = a; }
		virtual void SetCameraFade(float, float) {}
		virtual void SetCameraFadeEnabled(bool) {}
		virtual bool IsDrawEnable() const { return drawEnable_; }
		virtual void SetDrawEnable(bool isEnable) { drawEnable_ = isEnable; }
		virtual void SetPosition(const CalyxEngine::Vector3& pos) { position_ = pos; }
		virtual void DrawEmitterShape(const WorldTransform&) {}
		virtual void DrawEmitterShapePreview(const WorldTransform&) {}
		virtual const Guid& GetTextureGuid() const { return emptyGuid_; }
		virtual bool LoadTextureByGuid(const Guid&) { return false; }
		virtual void SetTextureGuid(const Guid&) {}

		/**
		 * \brief GUIDを用いてモデルデータをロード
		 * \param g
		 * \return
		 */
		virtual bool LoadModelByGuid(const Guid& g);

		void         SetAutoDestroy(bool autoDestroy) { autoDestroy_ = autoDestroy; }

	public:
		// accessor -------------------------------------------------
		virtual CalyxEngine::Vector3						GetWorldPosition() const { return position_; }
		const std::string&								GetTexturePath() const { return material_.texturePath; }
		const ParticleMaterial&							GetMaterial() const { return material_; }
		const DxConstantBuffer<ParticleMaterial>&		GetMaterialBuffer() const { return materialBuffer_; }
		const DxStructuredBuffer<ParticleConstantData>& GetInstanceBuffer() const { return instanceBuffer_; }
		const std::string&								GetModelPath() const { return modelPath; }
		const Guid&										GetModelGuid() const { return modelGuid_; }

		void									 SetPrimitive(ParticlePrimitives primitive) { primitive_ = primitive; }
		const std::optional<ParticlePrimitives>& GetPrimitive() const { return primitive_; }
		void SetEmitterShape(EmitterShape shape){shape_ = shape;}
		EmitterShape GetEmitterShape() const { return shape_; }
		void SetShapeSize(const CalyxEngine::Vector3& size) { shapeSize_ = size; }
		const CalyxEngine::Vector3& GetShapeSize() const { return shapeSize_; }
		void SetShapeRadius(float radius) { shapeRadius_ = radius; }
		float GetShapeRadius() const { return shapeRadius_; }
		void SetShapeAngle(float angleDeg) { shapeAngle_ = angleDeg; }
		float GetShapeAngle() const { return shapeAngle_; }
		void SetWorldRotation(const CalyxEngine::Quaternion& rotation) { worldRotation_ = rotation; }
		const CalyxEngine::Quaternion& GetWorldRotation() const { return worldRotation_; }
		void SetWorldScale(const CalyxEngine::Vector3& scale) { worldScale_ = scale; }
		const CalyxEngine::Vector3& GetWorldScale() const { return worldScale_; }

	protected:
		//===================================================================*/
		//					protected variable
		//===================================================================*/
		EmitterShape shape_ = EmitterShape::Point;
		CalyxEngine::Vector3 shapeSize_ = {1.0f,1.0f,1.0f}; // Boxなど
		float shapeRadius_ = 1.0f;                        // Sphereなど
		float shapeAngle_  = 30.0f;                       // Cone
		CalyxEngine::Quaternion worldRotation_ = CalyxEngine::Quaternion::MakeIdentity();
		CalyxEngine::Vector3 worldScale_ = {1.0f, 1.0f, 1.0f};

		std::optional<ParticlePrimitives>		 primitive_ = std::nullopt; //< プリミティブ形状(primitiveで発生する場合)
		MeshResource							 meshData_;					//< モデルデータ(使用しない場合はnull)
		std::string								 modelPath = "plane.obj";	//< モデルパス（デフォルトは平面
		Guid									 modelGuid_{Guid::Empty()};
		CalyxEngine::Vector3						 position_; //< emitterの位置
		ParticleMaterial						 material_; //< パーティクルのマテリアル
		std::vector<FxUnit>						 units_;	//< パーティクルユニットの配列
		DxStructuredBuffer<ParticleConstantData> instanceBuffer_;
		DxConstantBuffer<ParticleMaterial>		 materialBuffer_; // パーティクルマテリアルの定数バッファ

		float alphaMultiplier_ = 1.0f;
		bool  drawEnable_ = true;
		bool autoDestroy_ = false;

	private:
		inline static const Guid emptyGuid_{Guid::Empty()};
	};

} // namespace CalyxEngine
