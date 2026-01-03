#include "HudMotionBuilder.h"

namespace Calyx2D {
	namespace {
		CalyxEase::EaseType ToEase(int32_t easeInt) { return static_cast<CalyxEase::EaseType>(easeInt); }
	}

	void Calyx2D::BuildMotionSetFromFlatConfig(const HudTransformMotionConfig& cfg,HudMotionSet& out) {
		// Position
		if(cfg.posEnabled) {
			out.position = HudMotionDesc<CalyxMath::Vector2>{
					.start = cfg.posStart,
					.end = cfg.posEnd,
					.duration = cfg.posDuration,
					.easing = ToEase(cfg.posEaseInt)
				};
		} else { out.position.reset(); }

		// Scale
		if(cfg.scaleEnabled) {
			out.scale = HudMotionDesc<CalyxMath::Vector2>{
					.start = cfg.scaleStart,
					.end = cfg.scaleEnd,
					.duration = cfg.scaleDuration,
					.easing = ToEase(cfg.scaleEaseInt)
				};
		} else { out.scale.reset(); }

		// Rotation
		if(cfg.rotEnabled) {
			out.rotation = HudMotionDesc<float>{
					.start = cfg.rotStart,
					.end = cfg.rotEnd,
					.duration = cfg.rotDuration,
					.easing = ToEase(cfg.rotEaseInt)
				};
		} else { out.rotation.reset(); }

		// Alpha
		if(cfg.alphaEnabled) {
			out.alpha = HudMotionDesc<float>{
					.start = cfg.alphaStart,
					.end = cfg.alphaEnd,
					.duration = cfg.alphaDuration,
					.easing = ToEase(cfg.alphaEaseInt)
				};
		} else { out.alpha.reset(); }
	}

	void BuildExitMotionSetFromFlatConfig(const HudTransformMotionConfig& cfg, HudMotionSet& out) {
		HudTransformMotionConfig tmp = cfg;

		// Position
		std::swap(tmp.posStart, tmp.posEnd);
		// Scale
		std::swap(tmp.scaleStart, tmp.scaleEnd);
		// Rotation
		std::swap(tmp.rotStart, tmp.rotEnd);
		// Alpha
		std::swap(tmp.alphaStart, tmp.alphaEnd);

		BuildMotionSetFromFlatConfig(tmp, out);
	}
}