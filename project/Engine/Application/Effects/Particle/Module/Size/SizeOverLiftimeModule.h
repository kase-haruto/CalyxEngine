#pragma once

/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>
#include <Engine/Foundation/Utility/Ease/CxEase.h>

/// <summary>
/// ライフタイムに応じてサイズ変更
/// </summary>
class SizeOverLiftimeModule
	: public BaseFxModule {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	SizeOverLiftimeModule(const std::string name);
	~SizeOverLiftimeModule() override = default;

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="適応unit"></param>
	/// <param name="deltaTime"></param>
	void OnUpdate(struct FxUnit& unit, float dt) override;

	/// <summary>
	/// gui
	/// </summary>
	void ShowGuiContent() override;

	// accessor
	void SetIsGrowing(bool frag) { isGrowing_ = frag; }
	void SetEaseType(Cx::Ease::EaseType type) { easeType_ = type; }
	bool GetIsGrowing() const { return isGrowing_; }

	Cx::Ease::EaseType GetEaseType() const { return easeType_; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	bool isGrowing_ = true; //< サイズが大きくなるかどうか

	Cx::Ease::EaseType easeType_ = Cx::Ease::EaseType::EaseInOutCubic; //< サイズ変化のイージングタイプ
};