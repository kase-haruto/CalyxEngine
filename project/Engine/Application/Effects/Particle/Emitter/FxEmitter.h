#pragma once
/* ========================================================================
/*	include space
/* ===================================================================== */

// engine

#include <Engine/Application/Effects/Particle/Emitter/BaseEmitter.h>
#include <Engine/Application/Effects/Particle/Module/Container/FxModuleContainer.h>
#include <Engine/Application/Effects/Particle/Parm/FxParm.h>
#include <Engine/Objects/3D/Details/BillboardParams.h>

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
	~FxEmitter()override;

	virtual void Update(float dt) override;
	void		 TransferParticleDataToGPU();
	void		 ShowGui();

	// コマンドを積む
	void SetCommand(ID3D12GraphicsCommandList* cmdList);

	// particleUnit のリセット
	void ResetFxUnit(FxUnit& fxUnit);

	void Play() override; //< 再生
	void Stop() override; //< ストップ
	void Reset();         //< リセット
	bool LoadTextureByGuid(const Guid& g);

	//--------- config -------------------------------------------------//
	// 適用
	void ApplyConfigFrom(const EmitterConfig& config)override;
	// 掃き出し
	void ExtractConfigTo(EmitterConfig& config) const override;

	//--------- accessor -----------------------------------------------//
	const std::vector<FxUnit>& GetUnits() const { return units_; }

	bool							   IsDrawEnable() { return isDrawEnable_; }
	void							   SetDrawEnable(bool isEnable) { isDrawEnable_ = isEnable; }
	bool							   IsPlaying() const override { return isPlaying_; }
	const D3D12_GPU_DESCRIPTOR_HANDLE& GetTextureHandle() const { return textureHandle_; }

	//--------- Timed Preview（一定間隔での自動再生） ---------------//
	void  SetTimedPreview(bool v) { timedPreview_ = v; }
	void SetPosition(const Vector3& pos){position_ = pos;}
	bool  GetTimedPreview() const { return timedPreview_; }
	void  SetPreviewInterval(float sec) { previewIntervalSec_ = (sec < 0.01f ? 0.01f : sec); }
	float GetPreviewInterval() const { return previewIntervalSec_; }

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
	void RestartOneShot();

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
	FxParam<float>	 spin_;	 //< パーティクルのスピン（定数またはランダム）

protected:
	//===================================================================*/
	//					private variable
	//===================================================================*/

	const int					kMaxUnits_ = 4096; //< 最大パーティクル数
	D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_;
	Guid						textureGuid_;

	std::unique_ptr<FxModuleContainer> moduleContainer_; // モジュールコンテナ

	bool isPlaying_	   = true;	//< エフェクト再生中
	bool isFirstFrame_ = true;	//< 最初のフレーム
	bool isComplement_ = true;	//< trailするか
	bool isDrawEnable_ = true;	//< 描画するか
	bool randomSpinEmit_ = false; // emit時にスピン使用するか
	
protected:
	bool isOneShot_	  = false; //<
	bool hasEmitted_  = false; //< 発生したか
	bool autoDestroy_ = false; //< 自動削除するか
	int	 emitCount_	  = 10;

	float emitTimer_	= 0.0f; // パーティクル生成タイマー
	float emitDelay_	= 0.0f;
	float emitDuration_ = -1.0f;
	float elapsedTime_	= 0.0f;

	// === 一定間隔プレビュー用 ===
	bool  timedPreview_		  = false; // 1秒毎などで自動再生
	float previewIntervalSec_ = 1.0f;  // 既定 1 秒
	float previewTimer_		  = 0.0f;  // 経過タイマ

	// === billBoard ===
	GpuBillboardParams					 billboardParams_{};
	DxConstantBuffer<GpuBillboardParams> billboardCB_;
	BillboardMode						 billboardMode_ = BillboardMode::Full;

	std::function<void()> onFinished_;				   // 終了時コールバック
	bool				  isFinishedNotified_ = false; // すでに通知したかどうか
};