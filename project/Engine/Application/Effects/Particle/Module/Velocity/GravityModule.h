#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>

namespace CalyxEffect {
	/* ========================================================================
	/*		重力適用モジュール
	/* ===================================================================== */
	class GravityModule
		: public CalyxEffect::BaseFxModule {
	public:
		//===================================================================*/
		//					public methods
		//===================================================================*/
		GravityModule(const std::string name);
		~GravityModule() override = default;

		void OnUpdate(struct FxUnit& unit, float dt) override;
		void ShowGuiContent() override;

		//--------- getters -----------------------------------------------------
		const CalyxMath::Vector3 GetGravity() const { return gravity_; }

		//--------- setters -----------------------------------------------------
		void				SetGravity(const CalyxMath::Vector3& grav) { gravity_ = grav; }
		virtual const char* GetTypeName() const override { return "GravityModule"; }

	private:
		//===================================================================*/
		//					private methods
		//===================================================================*/
		CalyxMath::Vector3 gravity_{0.0f, -9.8f, 0.0f}; //< 重力の強さ
	};
} // namespace CalyxEffect