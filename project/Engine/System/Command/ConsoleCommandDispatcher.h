#pragma once

// engine
#include <Engine/System/Command/ConsoleCommandRegistry.h>

// c++
#include <string>
#include <vector>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * ConsoleCommandDispatcher
	 * - TerminalPanelから入力分割とコマンド実行の責務を分離するディスパッチャー
	 * - 引用符を含む引数を解析し、ConsoleCommandRegistryへ実行を委譲する
	 *---------------------------------------------------------------------------------------*/
	class ConsoleCommandDispatcher {
	public:
		ConsoleCommandDispatcher();

		ConsoleCommandResult Execute(
			const std::string& commandLine,
			const ConsoleCommandContext& context) const;
		std::vector<ConsoleCommandDefinition> GetCommandSuggestions(const std::string& prefix) const;

	private:
		void RegisterBuiltInCommands();
		static bool Tokenize(
			const std::string& commandLine,
			std::vector<std::string>& outTokens,
			std::string& outError);
	};

} // namespace CalyxEngine
