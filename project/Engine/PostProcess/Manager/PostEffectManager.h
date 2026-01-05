#pragma once

/* ========================================================================
/*  include space
/* ===================================================================== */
// engine
#include <Engine/PostProcess/Collection/PostProcessCollection.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
#include <Engine/Graphics/Device/DxCore.h>
#include <Engine/Graphics/GpuResource/DxGpuResource.h>
#include <Engine/Graphics/RenderTarget/Interface/IRenderTarget.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

// local
#include "../Graph/PostEffectGraph.h"

// c++
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <cmath>

class PipelineService;
class IPostEffectPass;

class PostEffectManager{
public:
	static PostEffectManager* Get();

	void Initialize(PipelineService* service, bool enableAll = false);

	bool IsInitialized() const{ return initialized_; }

	// ---------- トグルAPI ----------
	void Enable(const std::string& name, bool enabled = true);
	void Disable(const std::string& name){ Enable(name,false); }
	void Toggle(const std::string& name);
	bool IsEnabled(const std::string& name) const;

	void EnableOnly(std::initializer_list<std::string> names);
	void EnableAll(); // CopyImage は常にOFF
	void DisableAll();

	// ---------- 並び順 ----------
	bool MoveUp(const std::string& name);
	bool MoveDown(const std::string& name);
	void SetOrder(const std::vector<std::string>& orderedNames);

	// ---------- 実行/更新 ----------
	void Update(float dt);

	void Execute(ID3D12GraphicsCommandList* cmd,
				 DxGpuResource* input,
				 IRenderTarget* finalTarget,
				 CalyxGraphics::DxCore* dxCore);

	void TweenFloat(const std::string& passName,
					std::function<float()> getter,
					std::function<void(float)> setter,
					std::optional<float> from,
					float to,
					float durationSec,
					CalyxEase::EaseType ease = CalyxEase::EaseType::EaseOutSine,
					bool autoDisableIfZero = true,
					std::function<void()> onComplete = nullptr);

	// 直接パスを触りたい場合
	IPostEffectPass* GetPass(const std::string& name);

	void DrawImGui();

	// スロット参照
	const std::vector<PostEffectSlot>& GetSlots() const{ return collection_.GetSlots(); }
	std::vector<PostEffectSlot>& GetSlots(){ return collection_.GetSlots(); }

private:
	PostEffectManager() = default;

	int IndexOf(const std::string& name) const;
	void MarkDirty(){ dirty_ = true; }
	void RebuildGraphIfDirty();

private:
	struct FloatTween{
		std::string passName;
		std::function<float()> getter;
		std::function<void(float)> setter;
		float start = 0.f;
		float end = 0.f;
		float t = 0.f;
		float dur = 1.f;
		CalyxEase::EaseType ease = CalyxEase::EaseType::Linear;
		bool autoDisableIfZero = true;
		std::function<void()> onComplete;
	};

	std::vector<FloatTween> floatTweens_;

	bool initialized_ = false;
	bool dirty_ = true;

	CalyxGraphics::DxCore* dxCore_ = nullptr;

	PostProcessCollection collection_;
	PostEffectGraph graph_{&collection_};

	const std::string kCopyImageName = "CopyImage";
};