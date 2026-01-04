#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Ease/EaseTypes.h>

class SizeOverLiftimeModule
	:public BaseFxModule{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	SizeOverLiftimeModule(const std::string name);
	~SizeOverLiftimeModule()override = default;
	void OnUpdate(struct FxUnit& unit, float dt) override;
	void ShowGuiContent() override;

	void SetIsGrowing(bool frag) { isGrowing_ = frag; }
	void SetEaseType(EaseType type) { easeType_ =type; }

	bool GetIsGrowing() const { return isGrowing_; }
	EaseType GetEaseType() const { return easeType_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	bool isGrowing_ = true;			//< サイズが大きくなるかどうか
	EaseType easeType_ = EaseType::EaseInOutCubic;	//< サイズ変化のイージングタイプ
};

