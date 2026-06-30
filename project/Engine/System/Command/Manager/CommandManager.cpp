#include "CommandManager.h"

#include <Engine/Foundation/Log/EngineLogger.h>

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
		CalyxEngine::EngineLogger::GetInstance().Add(
			CalyxEngine::LogLevel::Info,
			CalyxEngine::LogCategory::Editor,
			std::string("Command executed: ") + name,
			"CommandManager");
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
	CalyxEngine::EngineLogger::GetInstance().Add(
		CalyxEngine::LogLevel::Info,
		CalyxEngine::LogCategory::Editor,
		std::string("Command undone: ") + cmd->GetName(),
		"CommandManager");
	redoStack_.push(std::move(cmd));
}

void CommandManager::Redo(){
	if (!CanRedo()) return;

	auto cmd = std::move(redoStack_.top());
	redoStack_.pop();

	cmd->Redo();

	commandLogs_.emplace_back(std::string("Redo: ") + cmd->GetName());
	CalyxEngine::EngineLogger::GetInstance().Add(
		CalyxEngine::LogLevel::Info,
		CalyxEngine::LogCategory::Editor,
		std::string("Command redone: ") + cmd->GetName(),
		"CommandManager");
	undoStack_.push(std::move(cmd));
}

void CommandManager::ClearHistory() {
	while(!undoStack_.empty()) {
		undoStack_.pop();
	}
	while(!redoStack_.empty()) {
		redoStack_.pop();
	}
	commandLogs_.clear();
}
