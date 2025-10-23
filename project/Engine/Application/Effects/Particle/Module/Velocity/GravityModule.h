#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Application/Effects/Particle/Module/BaseFxModule.h>
#include <Engine/Foundation/Math/Vector3.h>

/// <summary>
/// 重力適用
/// </summary>
class GravityModule
	:public BaseFxModule{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	GravityModule(const std::string name);
	~GravityModule()override = default;

	// 更新
	void OnUpdate(struct FxUnit& unit, float dt) override;
	
	// debugGui
	void ShowGuiContent() override;

	// accessor
	const Vector3 GetGravity()const { return gravity_; }
	void SetGravity(const Vector3& grav) { gravity_ = grav; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	Vector3 gravity_ {0.0f,-9.8f,0.0f};		//< 重力の強さ
};

