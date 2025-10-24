#pragma once
/* ========================================================================
/*  include space
/* ===================================================================== */
// engine
#include <Engine/Application/Effects/Particle/Emitter/FxEmitter.h>
#include <Engine/Application/Effects/Particle/Emitter/GpuFxEmitter.h>
#include <Engine/Renderer/Particle/ParticleRenderer.h>
#include <Engine/System/Event/EventBus.h>

// c++
#include <memory>
#include <vector>

struct Guid;

/* ========================================================================
/*  effect system
/* ===================================================================== */
class FxSystem {
public:
	//===================================================================*/
	//                  public func
	//===================================================================*/
	FxSystem();
	~FxSystem();

	/// <summary>
	/// emitter追加
	/// </summary>
	/// <param name="emitter"></param>
	void AddEmitter(const std::shared_ptr<BaseEmitter>& emitter);

	/// <summary>
	/// emitter削除
	/// </summary>
	/// <param name="emitter"></param>
	void RemoveEmitter(BaseEmitter* emitter);

	/// <summary>
	/// エミッタgpu同期
	/// </summary>
	void SyncEmitters();

	/// <summary>
	/// エミッタgpu同期
	/// </summary>
	/// <param name="psoService"></param>
	/// <param name="cmdList"></param>
	void DispatchEmitters(class PipelineService* psoService, ID3D12GraphicsCommandList* cmdList);

	/// <summary>
	/// 描画
	/// </summary>
	/// <param name=""></param>
	/// <param name=""></param>
	void Render(class PipelineService*, ID3D12GraphicsCommandList*);

	/// <summary>
	/// クリア
	/// </summary>
	void Clear();

private:
	//===================================================================*/
	//                  private helper
	//===================================================================*/
	/// <summary>
	/// guidから削除
	/// </summary>
	/// <param name="id"></param>
	void RemoveEmitterByGuid(const Guid& id);

private:
	//===================================================================*/
	//                  private variable
	//===================================================================*/
	std::vector<std::weak_ptr<FxEmitter>>	 cpuEmitters_;		//< cpuエミッタ
	std::vector<std::weak_ptr<GpuFxEmitter>> gpuEmitters_;		//< gpuエミッタ
	std::unique_ptr<ParticleRenderer>		 particleRenderer_;	//< レンダラ

	EventBus::Connection connAdd_;	//< 追加イベント
	EventBus::Connection connRem_;	//< 削除イベント
};
