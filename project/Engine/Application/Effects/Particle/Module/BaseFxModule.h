#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <string>

class BaseFxModule{
public:
	//===================================================================*/
	//					public methods
	//===================================================================*/
	BaseFxModule(const std::string name);
	virtual ~BaseFxModule() = default;

	virtual void ShowGuiContent() = 0;
	virtual void OnEmit(struct FxUnit&){}
	virtual void OnUpdate(struct FxUnit&, float){}

	bool IsEnabled() const{ return isEnabled_; }
	void SetEnabled(bool value){ isEnabled_ = value; }
	const std::string& GetName()const{ return name_; }
	void SetName(const std::string& n){ name_ = n; } 
protected:
	//===================================================================*/
	//					protected methods
	//===================================================================*/
	bool isEnabled_ = true; // モジュールが有効かどうか
	std::string name_ = "Module";
};

