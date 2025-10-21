#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <Engine/Objects/3D/Actor/SceneObject.h>
#include <Engine/Objects/Collider/Collider.h>

class BaseEventObject :
	public SceneObject {
public:
	//===================================================================*/
	//				 public methods
	//===================================================================*/
	BaseEventObject();
	~BaseEventObject() override;

	// 更新
	virtual void Update(float deltaTime) override;	//< runtimeのみ更新
	virtual void AlwaysUpdate(float dt) override;	//< 常時更新

	// gui
	virtual void ShowGui() override;

	// save/load
	virtual bool Save() const override;
	virtual bool Load() override;

	// collision
	virtual void OnCollisionEnter([[maybe_unused]] Collider* other) {}
	virtual void OnCollisionStay([[maybe_unused]] Collider* other) {}
	virtual void OnCollisionExit([[maybe_unused]] Collider* other) {}

protected:
	//===================================================================*/
	//				 protected methods
	//===================================================================*/
	std::unique_ptr<Collider> collider_ = nullptr;
};