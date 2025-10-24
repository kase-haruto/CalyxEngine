#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */

// engine
#include <Data/Engine/Configs/Scene/Objects/Particle/EmitterConfig.h>
#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
#include <Engine/Application/Effects/Particle/Module/Container/FxModuleContainer.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>

// c++
#include <functional>
#include <vector>

// forward declaration
struct Vector3;

/* ========================================================================
/*	particle emitter
/* ===================================================================== */
class FxEmitter : public BaseEmitter {
public:
	//===================================================================*/
	//					public func
	//===================================================================*/
	FxEmitter();
	~FxEmitter();

	virtual void Update(float dt) override;
	void ShowGui();

	// particleUnit のリセット
	void ResetFxUnit(FxUnit& fxUnit);

	void Play() override; //< 再生
	void Stop() override; //< ストップ
	void Reset();		  //< リセット

	//--------- config -------------------------------------------------//
	// 適用
	void ApplyConfigFrom(const EmitterConfig& config);
	// 掃き出し
	void ExtractConfigTo(EmitterConfig& config) const;

	//--------- accessor -----------------------------------------------//
	const std::vector<FxUnit>& GetUnits() const { return units_; }

	bool IsDrawEnable() { return isDrawEnable_; }
	void SetDrawEnable(bool isEnable) { isDrawEnable_ = isEnable; }
	bool IsPlaying() const override { return isPlaying_; }

	//--------- callback -----------------------------------------------//

	/// <summary>
	/// 再生終了コールバック
	/// </summary>
	/// <param name="callback"></param>
	void SetOnFinishedCallback(std::function<void()> callback);

private:
	//===================================================================*/
	//					private func
	//===================================================================*/
	// 発生
	void Emit();
	void Emit(const Vector3& pos);

public:
	//===================================================================*/
	//					public variable
	//===================================================================*/
	Vector3 prevPostion_;		 //< 前回の座標
	float	emitRate_	 = 0.1f; //< パーティクル生成レート
	float	defaultSize_ = 1.0f; //< パーティクルのデフォルトサイズ

	FxParam<Vector3> scale_;	//< パーティクルのスケール（定数またはランダム）
	FxParam<Vector3> velocity_; //< パーティクルの速度（定数またはランダム）
	FxParam<float>	 lifetime_; //< パーティクルの寿命（定数またはランダム）

private:
	//===================================================================*/
	//					private variable
	//===================================================================*/

	const int kMaxUnits_ = 4096; //< 最大パーティクル数

	std::unique_ptr<FxModuleContainer> moduleContainer_; // モジュールコンテナ

	bool isPlaying_	   = true;	//< エフェクト再生中
	bool isFirstFrame_ = true;	//< 最初のフレーム
	bool isComplement_ = true;	//< trailするか
	bool isStatic_	   = false; //< 静止か
	bool isDrawEnable_ = true;	//< 描画するか

private:
	bool isOneShot_	  = false; //<
	bool hasEmitted_  = false; //< 発生したか
	bool autoDestroy_ = false; //< 自動削除するか
	int	 emitCount_	  = 10;

	float emitTimer_	= 0.0f; // パーティクル生成タイマー
	float emitDelay_	= 0.0f;
	float emitDuration_ = -1.0f;
	float elapsedTime_	= 0.0f;

	std::function<void()> onFinished_;				   // 終了時コールバック
	bool				  isFinishedNotified_ = false; // すでに通知したかどうか
};
