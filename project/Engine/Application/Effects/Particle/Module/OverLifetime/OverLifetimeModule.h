#pragma once
/* ========================================================================
   OverLifetimeModule : lifetimeに応じて (start→end) を補間してプロパティへ適用
   ===================================================================== */
#include <Engine/Application/Effects/Particle/FxUnit.h>
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector4.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <string>

namespace CalyxEffect {
	class OverLifetimeModule final
		: public BaseFxModule {
	public:
		enum class Target {
			Scale = 0, // CalyxMath::Vector3
			RotationX, // float (deg)
			RotationY, // float (deg)
			RotationZ, // float (deg)
			ColorRGBA, // CalyxMath::Vector4
			AlphaOnly  // float (color.w)
		};
		enum class BlendOp {
			Set = 0,
			Add,
			Multiply
		};

		explicit OverLifetimeModule(const std::string& name = "OverLifetime");

		// BaseFxModule
		void OnUpdate(FxUnit& unit, float dt) override;
		void ShowGuiContent() override;

		// --- accessor ---
		void				SetTarget(Target t) { target_ = t; }
		Target				GetTarget() const { return target_; }
		void				SetBlend(BlendOp b) { blend_ = b; }
		BlendOp				GetBlend() const { return blend_; }
		void				SetEaseType(CalyxEase::EaseType e) { ease_ = e; }
		CalyxEase::EaseType	GetEaseType() const { return ease_; }
		void				SetClamp01(bool v) { clamp01_ = v; }
		bool				GetClamp01() const { return clamp01_; }
		void				SetInvert(bool v) { invert_ = v; }
		bool				GetInvert() const { return invert_; }
		virtual const char* GetTypeName() const override { return "OverLifetimeModule"; }
		void				SetStart(const CalyxMath::Vector4& v) { start_ = v; }
		void				SetEnd(const CalyxMath::Vector4& v) { end_ = v; }
		CalyxMath::Vector4	GetStart() const { return start_; }
		CalyxMath::Vector4	GetEnd() const { return end_; }

	private:
		void ApplyTo(FxUnit& u, const CalyxMath::Vector4& v) const;
		void DrawValueEditor(const char* label, CalyxMath::Vector4& v);

	private:
		Target			   target_	= Target::Scale;
		BlendOp			   blend_	= BlendOp::Set;
		CalyxEase::EaseType ease_	= CalyxEase::EaseType::EaseInOutCubic;
		bool			   clamp01_ = true;
		bool			   invert_	= false;

		CalyxMath::Vector4 start_{0, 0, 0, 1};
		CalyxMath::Vector4 end_{1, 1, 1, 1};
	};
}