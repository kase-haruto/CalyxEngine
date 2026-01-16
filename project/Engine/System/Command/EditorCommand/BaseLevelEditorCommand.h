#pragma once

#include <string>
#include <Engine/System/Command/Interface/ICommand.h>

/*-----------------------------------------------------------------------------------------
 * BaseLevelEditorCommand
 * - レベルエディタコマンド基底クラス
 * - エディタ操作のUndo/Redo対応コマンドの共通基底
 *---------------------------------------------------------------------------------------*/
class BaseLevelEditorCommand 
	: public ICommand {
public:
	explicit BaseLevelEditorCommand(const char* name = "Unnamed Command")
		: name_(name) {}

	// ICommand
	const char* GetName() const override final { return name_.c_str(); }
	void Redo() override { Execute(); }

	// 派生クラスで実装
	virtual void Execute() = 0;
	virtual void Undo() = 0;

	virtual ~BaseLevelEditorCommand() = default;

protected:
	std::string name_;
};