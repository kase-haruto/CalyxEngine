#pragma once

#include <Engine/System/Command/Interface/ICommand.h>

#include <stack>
#include <memory>
#include <vector>
#include <string>

/*-----------------------------------------------------------------------------------------
 * CommandManager
 * - コマンドマネージャークラス
 * - Undo/Redoスタックの管理とコマンドの実行・履歴管理を担当
 *---------------------------------------------------------------------------------------*/
class CommandManager {
public:
	static CommandManager* GetInstance();

	void Execute(std::unique_ptr<ICommand> cmd);
	void Undo();
	void Redo();

	bool CanUndo() const { return !undoStack_.empty(); }
	bool CanRedo() const { return !redoStack_.empty(); }

	const std::vector<std::string>& GetLogs() const{
		return commandLogs_;
	}

private:
	CommandManager() = default;
	~CommandManager() = default;
	CommandManager(const CommandManager&) = delete;
	CommandManager& operator=(const CommandManager&) = delete;
	CommandManager(CommandManager&&) = delete;
	CommandManager& operator=(CommandManager&&) = delete;

private:
	std::stack<std::unique_ptr<ICommand>> undoStack_;
	std::stack<std::unique_ptr<ICommand>> redoStack_;

private:
	std::vector<std::string> commandLogs_;
};

