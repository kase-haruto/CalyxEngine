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
#include <Engine/Foundation/Export/CalyxAPI.h>

// local
#include "../Graph/PostEffectGraph.h"

// c++
#include <string>
#include <vector>
#include <functional>
#include <optional>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <externals/nlohmann/json.hpp>

class PipelineService;
class IPostEffectPass;

class PostEffectManager{
public:
	CALYX_API static PostEffectManager* Get();

	void Initialize(PipelineService* service, bool enableAll = false);

	bool IsInitialized() const{ return initialized_; }

	// ---------- トグルAPI ----------
	CALYX_API void Enable(const std::string& name, bool enabled = true);
	void Disable(const std::string& name){ Enable(name,false); }
	CALYX_API void Toggle(const std::string& name);
	CALYX_API bool IsEnabled(const std::string& name) const;

	CALYX_API void EnableOnly(std::initializer_list<std::string> names);
	CALYX_API void EnableAll(); // CopyImage は常にOFF
	CALYX_API void DisableAll();

	// ---------- 並び順 ----------
	CALYX_API bool MoveUp(const std::string& name);
	CALYX_API bool MoveDown(const std::string& name);
	CALYX_API void SetOrder(const std::vector<std::string>& orderedNames);

	CALYX_API bool SavePreset(const std::string& filePath, const std::string& presetName = "PostEffectPreset") const;
	CALYX_API bool LoadPreset(const std::string& filePath);
	// Adds a preset to the current scene composition. Unmentioned effects are
	// preserved and all slots are executed as one linear post-effect chain.
	CALYX_API bool MergePreset(const std::string& filePath);
	CALYX_API bool PlayTriggeredPreset(const std::string& filePath);
	CALYX_API void PlayTriggeredEffects();
	CALYX_API void PlayTriggeredEffect(const std::string& name);

	void SetOutlineEnabled(bool enabled) { outlineEnabled_ = enabled; }
	bool IsOutlineEnabled() const { return outlineEnabled_; }

	// ---------- 実行/更新 ----------
	void Update(float dt);

	void Execute(ID3D12GraphicsCommandList* cmd,
				 DxGpuResource* input,
				 IRenderTarget* finalTarget,
				 CalyxEngine::DxCore* dxCore);

	void TweenFloat(const std::string& passName,
					std::function<float()> getter,
					std::function<void(float)> setter,
					std::optional<float> from,
					float to,
					float durationSec,
					CalyxEngine::EaseType ease = CalyxEngine::EaseType::EaseOutSine,
					bool autoDisableIfZero = true,
					std::function<void()> onComplete = nullptr);

	// 直接パスを触りたい場合
	CALYX_API IPostEffectPass* GetPass(const std::string& name);

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
	struct OverlaySlotSnapshot {
		bool enabled = false;
		PostEffectApplyMode applyMode = PostEffectApplyMode::Always;
		float duration = 0.25f;
		CalyxEngine::EaseType ease = CalyxEngine::EaseType::Linear;
		bool autoDisable = true;
		std::vector<PostEffectFloatAnimation> floatAnimations;
		nlohmann::json parameters = nlohmann::json::object();
	};

	struct FloatTween{
		std::string passName;
		std::function<float()> getter;
		std::function<void(float)> setter;
		float start = 0.f;
		float end = 0.f;
		float t = 0.f;
		float dur = 1.f;
		CalyxEngine::EaseType ease = CalyxEngine::EaseType::Linear;
		bool autoDisableIfZero = true;
		std::function<void()> onComplete;
	};

	std::vector<FloatTween> floatTweens_;
	std::unordered_map<std::string, OverlaySlotSnapshot> overlaySnapshots_;
	std::unordered_map<std::string, uint64_t> overlayGenerations_;
	bool mergingPreset_ = false;

	bool initialized_ = false;
	bool dirty_ = true;

	CalyxEngine::DxCore* dxCore_ = nullptr;

	PostProcessCollection collection_;
	PostEffectGraph graph_{&collection_};
	nlohmann::json loadedPreset_;
	bool hasLoadedGraph_ = false;
	bool outlineEnabled_ = true;

	const std::string kCopyImageName = "CopyImage";
	const std::string kBlendName = "Blend";
	const std::string kDefaultPaht = "PostEffects/Default.postfx";
};
