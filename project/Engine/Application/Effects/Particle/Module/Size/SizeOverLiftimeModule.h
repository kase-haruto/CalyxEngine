#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

namespace CalyxEngine{
	/* ========================================================================
	/*	ライフタイムに応じてパーティクルのサイズを変化させるモジュール
	/* ===================================================================== */
	class SizeOverLiftimeModule
		: public CalyxEngine::BaseFxModule {
	public:
		//===================================================================*/
		//					public methods
		//===================================================================*/
		SizeOverLiftimeModule(const std::string name);
		~SizeOverLiftimeModule() override = default;

		void OnUpdate(struct FxUnit& unit, float dt) override;
		void ShowGuiContent() override;

		//--------- accessor -----------------------------------------------------
		// getter
		void SetIsGrowing(bool frag) { isGrowing_ = frag; }
		void SetEaseType(CalyxEngine::EaseType type) { easeType_ = type; }

		// setter
		bool				GetIsGrowing() const { return isGrowing_; }
		CalyxEngine::EaseType	GetEaseType() const { return easeType_; }
		virtual const char* GetObjectClassName() const override { return "SizeOverLiftimeModule"; }

	private:
		//===================================================================*/
		//					private methods
		//===================================================================*/
		bool isGrowing_ = true; //< サイズが大きくなるかどうか

		CalyxEngine::EaseType easeType_ = CalyxEngine::EaseType::EaseInOutCubic; //< サイズ変化のイージングタイプ
	};
}