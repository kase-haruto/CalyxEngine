#pragma once

// engine
#include <Engine/PostProcess/GrayScale/GrayScale.h>
#include <Engine/PostProcess/Blur/RadialBlur/RadialBlur.h>
#include <Engine/PostProcess/CopyImage/CopyImage.h>
#include <Engine/PostProcess/ChromaticAberration/ChromaticAberrationEffect.h>
#include <Engine/PostProcess/Slot/PostEffectSlot.h>
// c++
#include <memory>
#include <vector>

class PostProcessCollection {
public:
	void Initialize(class PipelineService* service);


	std::vector<PostEffectSlot>& GetSlots() { return slots_; }
	const std::vector<PostEffectSlot>& GetSlots() const { return slots_; }

	void BuildInitialSlots(bool enableAll = false) {
		slots_.clear();
		for (const auto& effect : effects_) {
			slots_.push_back({
				effect->GetName(),
				enableAll,
				effect.get()
							 });
		}
	}

	IPostEffectPass* GetEffectByName(const std::string& name) {
		for (const auto& effect : effects_) {
			if (effect->GetName() == name) {
				return effect.get();
			}
		}
		return nullptr;
	}

	const std::vector<std::string>& GetEffectNames() const { return effectNames_; }

private:
	std::vector<std::unique_ptr<IPostEffectPass>> effects_;
	std::vector<std::string> effectNames_;

	std::vector<PostEffectSlot> slots_;
};