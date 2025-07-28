#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */

// engine
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Engine/Application/Effects/Particle/Detail/ParticleDetail.h>
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Module/Container/FxModuleContainer.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Graphics/Buffer/DxConstantBuffer.h>
#include <Engine/Graphics/Buffer/DxStructuredBuffer.h>
#include <Engine/Graphics/Material.h>
#include <Engine/Objects/ConfigurableObject/ConfigurableObject.h>

// c++
#include <functional>
#include <vector>

// forward declaration
struct Vector3;

/* ========================================================================
/*	particle emitter
/* ===================================================================== */
class FxEmitter {
public:
	//===================================================================*/
	//					public func
	//===================================================================*/
	FxEmitter();
	~FxEmitter();

	virtual void Update();
	void ResetFxUnit(FxUnit& fxUnit);
	void ShowGui();
	void TransferParticleDataToGPU();

	void Play();
	void Stop();
	void Reset();

	//--------- config -------------------------------------------------//
	void ApplyConfigFrom(const EmitterConfig& config);
	void ExtractConfigTo(EmitterConfig& config) const;

	//--------- accessor -------------------------------------------------//
	const std::vector<FxUnit>& GetUnits()const{ return units_; }
	const std::string& GetModelPath() const{ return modelPath; }
	const std::string& GetTexturePath() const{ return material_.texturePath; }
	const ParticleMaterial& GetMaterial() const{ return material_; }
	const DxConstantBuffer<ParticleMaterial>& GetMaterialBuffer() const{ return materialBuffer_; }
	const DxStructuredBuffer<ParticleConstantData>& GetInstanceBuffer() const{ return instanceBuffer_; }
	bool IsDrawEnable(){ return isDrawEnable_; }
	void SetDrawEnable(bool isEnable){ isDrawEnable_ = isEnable; }
	bool isPlayng()const{ return isPlaying_; }

	//--------- callback -------------------------------------------------//
	void SetOnFinishedCallback(std::function<void()> callback){
		onFinished_ = std::move(callback);
	}

private:
	//===================================================================*/
	//					private func
	//===================================================================*/
	void Emit();
	void Emit(const Vector3& pos);

public:
	//===================================================================*/
	//					public variable
	//===================================================================*/
	Vector3 position_;					//< emitterの位置
	Vector3 prevPostion_;				//< 前回の座標
	float emitRate_ = 0.1f;				//< パーティクル生成レート
	float defaultSize_ = 1.0f;			//< パーティクルのデフォルトサイズ

	FxParam<Vector3> scale_;			//< パーティクルのスケール（定数またはランダム）
	FxParam<Vector3> velocity_;			//< パーティクルの速度（定数またはランダム）
	FxParam<float> lifetime_;			//< パーティクルの寿命（定数またはランダム）

private:
	//===================================================================*/
	//					private variable
	//===================================================================*/
	std::string modelPath = "plane.obj";			//< モデルパス（デフォルトは平面

	const int kMaxUnits_ = 2048;				//< 最大パーティクル数
	std::vector<FxUnit> units_;				//< パーティクルユニットの配列

	std::unique_ptr<FxModuleContainer> moduleContainer_;	// モジュールコンテナ

	float emitTimer_ = 0.0f; // パーティクル生成タイマー

	bool isPlaying_ = true;
	bool isFirstFrame_ = true;
	bool isComplement_ = true;
	bool isStatic_ = false;
	bool isDrawEnable_ = true;

private:
	bool isOneShot_ = false;
	bool hasEmitted_ = false;
	bool autoDestroy_ = false;
	int emitCount_ = 10;
	float emitDelay_ = 0.0f;
	float emitDuration_ = -1.0f;
	float elapsedTime_ = 0.0f;

	std::function<void()> onFinished_;   // 終了時コールバック
	bool isFinishedNotified_ = false;   // すでに通知したかどうか

private:
	//resources
	ParticleMaterial material_;	//< パーティクルのマテリアル
	DxStructuredBuffer<ParticleConstantData> instanceBuffer_;
	DxConstantBuffer<ParticleMaterial> materialBuffer_; // パーティクルマテリアルの定数バッファ
};
