#pragma once

#include <Engine/Graphics/Material.h>
#include <algorithm>

namespace CalyxEngine {
	class ShieldEffect {
	public:
		void Bind(Material* material) { material_ = material; Apply(); }
		void Update(float deltaTime) {
			if(!material_) return;
			elapsedTime_ += std::clamp(deltaTime, 0.0f, 0.1f);
			material_->shieldRipple.z = elapsedTime_;
		}
		void TriggerRipple(float phase = 0.0f) {
			elapsedTime_ = 0.0f;
			if(material_) material_->shieldRipple.w = phase;
		}
		void SetEnabled(bool enabled) { enabled_ = enabled; Apply(); }
		void Apply() {
			if(!material_) return;
			material_->shieldParams.x = enabled_ ? 1.0f : 0.0f;
		}
	private:
		Material* material_ = nullptr;
		float elapsedTime_ = 0.0f;
		bool enabled_ = true;
	};
}
