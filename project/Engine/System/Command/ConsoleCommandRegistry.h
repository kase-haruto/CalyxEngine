#pragma once

// engine
#include <Engine/Foundation/Export/CalyxAPI.h>

// c++
#include <functional>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace CalyxEngine {

	class LevelEditor;
	class PlaySession;
	class SceneManager;

	enum class ConsoleOutputLevel {
		Info,
		Warning,
		Error
	};

	/*-----------------------------------------------------------------------------------------
	 * ConsoleOutputLine
	 * - Consoleへ表示する一行分の実行結果を保持するデータ構造
	 * - 出力レベルと表示メッセージを管理する
	 *---------------------------------------------------------------------------------------*/
	struct ConsoleOutputLine {
		ConsoleOutputLevel level = ConsoleOutputLevel::Info;
		std::string message;
	};

	/*-----------------------------------------------------------------------------------------
	 * ConsoleCommandResult
	 * - Console Commandの実行結果を呼び出し側へ返すデータ構造
	 * - 履歴消去要求と複数行の出力内容を保持する
	 *---------------------------------------------------------------------------------------*/
	struct ConsoleCommandResult {
		bool clearRequested = false;
		std::vector<ConsoleOutputLine> output;
	};

	/*-----------------------------------------------------------------------------------------
	 * ConsoleCommandContext
	 * - Console Commandが参照可能なEngineサービスを明示するデータ構造
	 * - 各ポインタは所有権を持たず、Command実行中だけ利用する
	 *---------------------------------------------------------------------------------------*/
	struct ConsoleCommandContext {
		LevelEditor* levelEditor = nullptr;
		SceneManager* sceneManager = nullptr;
		PlaySession* playSession = nullptr;
	};

	using ConsoleCommandCallback = std::function<ConsoleCommandResult(
		const ConsoleCommandContext&,
		const std::vector<std::string>&)>;

	/*-----------------------------------------------------------------------------------------
	 * ConsoleCommandDefinition
	 * - Registryへ登録するConsole Commandの定義を保持するデータ構造
	 * - コマンド名、説明、利用形式、実行Callbackを管理する
	 *---------------------------------------------------------------------------------------*/
	struct ConsoleCommandDefinition {
		std::string name;
		std::string description;
		std::string usage;
		ConsoleCommandCallback callback;
	};

	/*-----------------------------------------------------------------------------------------
	 * ConsoleCommandRegistry
	 * - Engine/Gameからコマンド名とコールバックを登録する共通レジストリ
	 * - UIへ依存せず、コマンド検索と実行だけを担当する
	 *---------------------------------------------------------------------------------------*/
	class CALYX_API ConsoleCommandRegistry {
	public:
		static ConsoleCommandRegistry& GetInstance();

		bool Register(ConsoleCommandDefinition definition);
		bool Unregister(const std::string& name);

		ConsoleCommandResult Execute(
			const std::string& name,
			const std::vector<std::string>& arguments,
			const ConsoleCommandContext& context) const;

		std::optional<ConsoleCommandDefinition> Find(const std::string& name) const;
		std::vector<ConsoleCommandDefinition> GetCommands() const;

	private:
		static std::string NormalizeName(const std::string& name);

	private:
		mutable std::mutex mutex_;
		std::unordered_map<std::string, ConsoleCommandDefinition> commands_;
	};

} // namespace CalyxEngine
