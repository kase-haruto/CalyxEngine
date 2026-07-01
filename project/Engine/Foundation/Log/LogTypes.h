#pragma once

// c++
#include <chrono>
#include <cstdint>
#include <string>

namespace CalyxEngine {

	// ログの重要度を表す。
	enum class LogLevel {
		Trace,
		Info,
		Warning,
		Error
	};

	// ログの発生領域を表す。
	enum class LogCategory {
		Engine,
		Editor,
		Game,
		Asset,
		Rendering,
		Physics,
		Command
	};

	/*-----------------------------------------------------------------------------------------
	 * LogEntry
	 * - EngineLogger が保持するログ一件分のデータ
	 * - UIに依存せず、将来のファイル出力や外部出力でも共用する
	 *---------------------------------------------------------------------------------------*/
	struct LogEntry {
		std::uint64_t id = 0;
		LogLevel level = LogLevel::Info;
		LogCategory category = LogCategory::Engine;
		std::string message;
		std::string source;
		std::chrono::system_clock::time_point time;
	};

} // namespace CalyxEngine
