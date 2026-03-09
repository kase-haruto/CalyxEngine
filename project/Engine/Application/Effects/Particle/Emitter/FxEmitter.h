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
#include "Engine/Graphics/Pipeline/BlendMode/BlendMode.h"
// stl
#include <functional>
#include <vector>

// forward declaration
struct CalyxMath::Vector3;

namespace CalyxEffect {

	/*-----------------------------------------------------------------------------------------
	 * FxEmitter
	 * - CPUベースのパーティクルエミッタクラス
	 * - パーティクルの発生・寿命管理・モジュール適用・描画処理を担当
	 *---------------------------------------------------------------------------------------*/
	class FxEmitter
		: public BaseEmitter {
	public:
		//===================================================================*/
		//					public func
		//===================================================================*/
		FxEmitter();
		~FxEmitter() override;

		virtual void Update(float dt) override;
		void		 TransferParticleDataToGPU() override; // BaseEmitterをoverride
		void		 ShowGui();
		void DrawEmitterShape(const WorldTransform& tf);
		// コマンドを積む
		void SetCommand(ID3D12GraphicsCommandList* cmdList);

		// particleUnit のリセット
		void ResetFxUnit(FxUnit& fxUnit);

		void Play() override; //< 再生
		void Stop() override; //< ストップ
		void Reset();		  //< リセット
		bool LoadTextureByGuid(const Guid& g);

		void SetCameraFade(float nearZ, float farZ) override;

		//--------- config -------------------------------------------------//
		// 適用
		void ApplyConfigFrom(const EmitterConfig& config) override;
		// 掃き出し
		void ExtractConfigTo(EmitterConfig& config) const override;

		//--------- accessor -----------------------------------------------//
		const std::vector<FxUnit>& GetUnits() const { return units_; }

		bool							   IsDrawEnable() { return HasFlag(DrawEnable); }
		void							   SetDrawEnable(bool isEnable) { SetFlag(DrawEnable, isEnable); }
		bool							   IsPlaying() const override { return HasFlag(Playing); }
		const D3D12_GPU_DESCRIPTOR_HANDLE& GetTextureHandle() const { return textureHandle_; }

		//--------- Timed Preview（一定間隔での自動再生） ---------------//
		void	  SetTimedPreview(bool v) { timedPreview_ = v; }
		void	  SetPosition(const CalyxMath::Vector3& pos) { position_ = pos; }
		bool	  GetTimedPreview() const { return timedPreview_; }
		void	  SetPreviewInterval(float sec) { previewIntervalSec_ = (sec < 0.01f ? 0.01f : sec); }
		float	  GetPreviewInterval() const { return previewIntervalSec_; }
		BlendMode GetBlendMode() const { return blendMode_; }

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
		void Emit(const CalyxMath::Vector3& pos);
		void RestartOneShot();

		// ---- flags helpers ----
	private:
		enum FlagBits : uint32_t {
			None		   = 0,
			FollowOneShot  = 1u << 0,
			Playing		   = 1u << 1,
			FirstFrame	   = 1u << 2,
			Complement	   = 1u << 3,
			DrawEnable	   = 1u << 4,
			RandomSpinEmit = 1u << 5,
		};
		inline bool HasFlag(FlagBits f) const { return (flags_ & f) != 0; }
		inline void SetFlag(FlagBits f, bool enable) {
			if(enable)
				flags_ |= f;
			else
				flags_ &= ~f;
		}

	public:
		//===================================================================*/
		//					public variable
		//===================================================================*/
		CalyxMath::Vector3 prevPostion_;		//< 前回の座標
		float			   emitRate_	= 0.1f; //< パーティクル生成レート
		float			   defaultSize_ = 1.0f; //< パーティクルのデフォルトサイズ

		FxParam<CalyxMath::Vector3> scale_;	   //< パーティクルのスケール（定数またはランダム）
		FxParam<CalyxMath::Vector3> velocity_; //< パーティクルの速度（定数またはランダム）
		FxParam<float>				lifetime_; //< パーティクルの寿命（定数またはランダム）
		FxParam<float>				spin_;	   //< パーティクルのスピン（定数またはランダム）

	protected:
		//===================================================================*/
		//					private variable
		//===================================================================*/
		BlendMode					blendMode_ = BlendMode::ADD; //< ブレンドモード
		const int					kMaxUnits_ = 4096;			 //< 最大パーティクル数
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandle_{};			 // 初期化
		Guid						textureGuid_{};

		std::unique_ptr<FxModuleContainer> moduleContainer_; // モジュールコンテナ

		// 旧bool群を統合したビットフラグ
		uint32_t flags_ = FollowOneShot | FirstFrame | Complement | DrawEnable;

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

		// === camera fade ===
		GpuFadeParams					fadeParams_{};
		DxConstantBuffer<GpuFadeParams> fadeCB_;

		std::function<void()> onFinished_;				   // 終了時コールバック
		bool				  isFinishedNotified_ = false; // すでに通知したかどうか
	};
} // namespace CalyxEffect