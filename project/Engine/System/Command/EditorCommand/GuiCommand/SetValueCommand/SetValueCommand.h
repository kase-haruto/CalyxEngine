#pragma once

#include <Engine/System/Command/Interface/ICommand.h>

#include <functional>
#include <string>


template<typename T>
class SetValueCommand 
	: public ICommand{
public:
	SetValueCommand(const std::string& label, const T& before, const T& after, std::function<void(const T&)> setter)
		: label_(label), before_(before), after_(after), setter_(std::move(setter)){
		UpdateName();
	}

	void Execute() override{ setter_(after_); }
	void Undo() override{ setter_(before_); }
	void Redo() override{ Execute(); }

	const char* GetName() const override{ return name_.c_str(); }
	void SetName(const std::string& name){ name_ = name; }

private:
	void UpdateName();

private:
	std::string label_;
	T before_;
	T after_;
	std::function<void(const T&)> setter_;
	std::string name_ = "SetValueCommand"; // ログ用の名前
};

