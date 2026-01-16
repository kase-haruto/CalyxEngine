#pragma once

/*-----------------------------------------------------------------------------------------
 * ICommand
 * - コマンドインターフェース
 * - Undo/Redoパターンの実行・取り消し・やり直し操作を定義
 *---------------------------------------------------------------------------------------*/
class ICommand{
public:
	virtual ~ICommand() = default;

	virtual void Execute() = 0;
	virtual void Undo() = 0;
	virtual void Redo() { Execute(); }
	virtual const char* GetName() const = 0;
};

