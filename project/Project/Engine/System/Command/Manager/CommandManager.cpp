#include "CommandManager.h"

#include <string>

CommandManager* CommandManager::GetInstance() {
	static CommandManager instance;
	return &instance;
}

void CommandManager::Execute(std::unique_ptr<ICommand> cmd){
	if (!cmd) return;

	cmd->Execute();

	// ログ追加
	if (const char* name = cmd->GetName(); name && *name){
		commandLogs_.emplace_back(name);
	}

	undoStack_.push(std::move(cmd));
	while (!redoStack_.empty()) redoStack_.pop();
}

void CommandManager::Undo(){
	if (!CanUndo()) return;

	auto cmd = std::move(undoStack_.top());
	undoStack_.pop();

	cmd->Undo();

	commandLogs_.emplace_back(std::string("Undo: ") + cmd->GetName());
	redoStack_.push(std::move(cmd));
}

void CommandManager::Redo(){
	if (!CanRedo()) return;

	auto cmd = std::move(redoStack_.top());
	redoStack_.pop();

	cmd->Redo();

	commandLogs_.emplace_back(std::string("Redo: ") + cmd->GetName());
	undoStack_.push(std::move(cmd));
}