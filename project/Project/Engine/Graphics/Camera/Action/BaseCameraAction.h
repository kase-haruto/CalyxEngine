#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
#include <string>

class BaseCameraAction {
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BaseCameraAction();
	virtual ~BaseCameraAction();

	virtual void Update(class BaseCamera*, float) {}
	virtual void Execute() = 0;
	virtual void ShowGui() {}

	//---------- accessor ------------------------------------------------//
	// setter
	void SetActionName(const std::string& name);

	// getter
	const std::string& GetActionName() const;

protected:
	//===================================================================*/
	//					protected methods
	//===================================================================*/
	std::string actionName_ = "CameraAction";
};
