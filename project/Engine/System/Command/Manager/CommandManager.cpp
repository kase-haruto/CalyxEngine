#include "CommandManager.h"

#include <Engine/Foundation/Log/EngineLogger.h>

#include <string>

CommandManager* CommandManager::GetInstance() {
	static CommandManager instance;
	return &instance;
}

void CommandManager::Execute(std::unique_ptr<ICommand> cmd){
	if (!cmd) return;

	// Commandを先に適用し、成功した操作だけをUndo履歴へ移動する。
	cmd->Execute();

	// 操作名をEditorログへ残し、履歴UIと診断ログの双方から追跡可能にする。
	if (const char* name = cmd->GetName(); name && *name){
		commandLogs_.emplace_back(name);
		CalyxEngine::EngineLogger::GetInstance().Add(
			CalyxEngine::LogLevel::Info,
			CalyxEngine::LogCategory::Editor,
			std::string("Command executed: ") + name,
			"CommandManager");
	}

	undoStack_.push(std::move(cmd));
	// 新規操作後のRedo履歴は分岐前の状態を指すため、すべて無効化する。
	while (!redoStack_.empty()) redoStack_.pop();
}

void CommandManager::Undo(){
	if (!CanUndo()) return;

	// 所有権をUndo側から取り出し、復元後にRedo側へ移す。
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

	// 再適用したCommandをUndo側へ戻し、再び取り消せる状態にする。
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
	// Scene切替後に旧Sceneへ作用するCommandを実行できないよう、両履歴と表示ログを破棄する。
	while(!undoStack_.empty()) {
		undoStack_.pop();
	}
	while(!redoStack_.empty()) {
		redoStack_.pop();
	}
	commandLogs_.clear();
}
