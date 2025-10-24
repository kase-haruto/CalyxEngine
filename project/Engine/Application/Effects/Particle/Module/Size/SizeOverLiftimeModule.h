#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

/* ========================================================================
/*	ライフタイムに応じてパーティクルのサイズを変化させるモジュール
/* ===================================================================== */
class SizeOverLiftimeModule
	: public BaseFxModule {
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
	void SetEaseType(Cx::Ease::EaseType type) { easeType_ = type; }

	// setter
	bool			   GetIsGrowing() const { return isGrowing_; }
	Cx::Ease::EaseType GetEaseType() const { return easeType_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	bool isGrowing_ = true; //< サイズが大きくなるかどうか

	Cx::Ease::EaseType easeType_ = Cx::Ease::EaseType::EaseInOutCubic; //< サイズ変化のイージングタイプ
};