#pragma once

#include <Engine/System/Command/Interface/ICommand.h>

#include <functional>
#include <string>
#include <utility>

template<typename T>
class ValueEditCommand final
	: public ICommand {
public:
	ValueEditCommand(std::string name, T before, T after, std::function<void(const T&)> setter)
		: name_(std::move(name)), before_(std::move(before)), after_(std::move(after)), setter_(std::move(setter)) {}

	void Execute() override { setter_(after_); }
	void Undo() override { setter_(before_); }
	void Redo() override { Execute(); }
	const char* GetName() const override { return name_.c_str(); }

private:
	std::string name_;
	T before_;
	T after_;
	std::function<void(const T&)> setter_;
};
