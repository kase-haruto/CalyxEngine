#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */

// engine
#include <Engine/Foundation/Utility/Ease/CxEase.h>
#include <Engine/Objects/Event/Camera/CameraEventObject.h>

// config
#include <Data/Game/Config/Event/CameraTurnAroundEventConfig.h>

class CameraTurnAroundAction;

class CameraTurnAroundEvent final
	: public CameraEventObject {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	CameraTurnAroundEvent();
	CameraTurnAroundEvent(const std::string& name);
	~CameraTurnAroundEvent() override;

	void Initialize() override;
	void AlwaysUpdate(float dt) override;
	
	// 発火時処理
	void OnCollisionEnter(Collider* other) override;
	void OnCollisionExit(Collider* other) override;

	// 設定の保存ロード
	void ApplyConfig() override;
	void ExtractConfig() override;

	// debug ui
	void			 DerivativeGui() override;
	void			 ConfigGUi() override;
	std::string		 GetObjectTypeName() const override { return name_; }
	std::string_view GetTypeName() const override { return "CameraTurnAroundEvent"; }

private:
	//===================================================================*/
	//					private methods
	//===================================================================*/
	CalyxMath::Vector3			   direction_; //< 振り向く方向
	float			   time_;	   //< 振り向く時間
	CalyxEase::EaseType easeType_;  //< 使用イージングタイプ

	// config
	ConfigurableObject<CameraTurnAroundEventConfig> config_;

	// action
	std::unique_ptr<CameraTurnAroundAction> turnAction_;
	std::unique_ptr<CameraTurnAroundAction> returnAction_;
	CalyxMath::Vector3 originalDir_;
	bool active_ = false;
};