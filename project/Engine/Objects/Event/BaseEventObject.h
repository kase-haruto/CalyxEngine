#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Data/Engine/Configs/Scene/Objects/Event/EventConfig.h>

#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Collider/Collider.h>
#include <Engine/objects/ConfigurableObject/ConfigurableObject.h>

/* ========================================================================
/*		イベントをシーンオブジェクトとして配置
/* ===================================================================== */
class BaseEventObject :
	public SceneObject,
	public IConfigurable {
public:
	//===================================================================*/
	//				 public methods
	//===================================================================*/
	BaseEventObject();
	BaseEventObject(const std::string& name);
	~BaseEventObject() override;

	// 更新
	virtual void AlwaysUpdate(float dt) override; //< 常時更新

	// gui
	virtual void ShowGui() override;
	virtual void DerivativeGui();		//< 派生先のparameter調整

	// config
	virtual void ApplyConfig();
	virtual void ExtractConfig();
	void         ApplyConfigFromJson(const nlohmann::json& j) override;
	void         ExtractConfigToJson(nlohmann::json& j) const override;

	// collision
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) {}
	virtual void OnCollisionStay([[maybe_unused]] Collider* other) {}
	virtual void OnCollisionExit([[maybe_unused]] Collider* other) {}

protected:
	//===================================================================*/
	//				 protected methods
	//===================================================================*/
	std::unique_ptr<Collider> collider_ = nullptr;
	ConfigurableObject<EventConfig> config_;
};