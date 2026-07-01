#pragma once

// engine
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Log/LogTypes.h>

// c++
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * EngineLogger
	 * - エンジン全体から利用できるログ蓄積サービス
	 * - ImGuiに依存せず、出力先の追加をこのクラス側へ集約できる構成とする
	 *---------------------------------------------------------------------------------------*/
	class CALYX_API EngineLogger {
	public:
		static EngineLogger& GetInstance();

		void Add(LogLevel level,
				 LogCategory category,
				 const std::string& message,
				 const std::string& source = "");
		void Trace(const std::string& message, const std::string& source = "");
		void Info(const std::string& message, const std::string& source = "");
		void Warning(const std::string& message, const std::string& source = "");
		void Error(const std::string& message, const std::string& source = "");

		void Clear();

		// 呼び出し側が安全に参照できるよう、ロック中にスナップショットを作成する。
		std::vector<LogEntry> GetEntries() const;
		std::uint64_t GetRevision() const;

		void SetMaxEntries(std::size_t maxEntries);
		std::size_t GetMaxEntries() const;

	private:
		EngineLogger() = default;

		// 最大件数を超えた古いログを先頭から破棄する。
		void TrimToMaxEntries();

	private:
		mutable std::mutex mutex_;
		std::vector<LogEntry> entries_;
		std::size_t maxEntries_ = 1000;
		std::uint64_t nextEntryId_ = 1;
		std::uint64_t revision_ = 0;
	};

} // namespace CalyxEngine

// 呼び出し元の関数名をsourceとして記録する、最小限のログ出力マクロ。
#define CYLOG_TRACE(message) \
	::CalyxEngine::EngineLogger::GetInstance().Add(::CalyxEngine::LogLevel::Trace, ::CalyxEngine::LogCategory::Engine, (message), __FUNCTION__)
#define CYLOG_INFO(message) \
	::CalyxEngine::EngineLogger::GetInstance().Add(::CalyxEngine::LogLevel::Info, ::CalyxEngine::LogCategory::Engine, (message), __FUNCTION__)
#define CYLOG_WARN(message) \
	::CalyxEngine::EngineLogger::GetInstance().Add(::CalyxEngine::LogLevel::Warning, ::CalyxEngine::LogCategory::Engine, (message), __FUNCTION__)
#define CYLOG_ERROR(message) \
	::CalyxEngine::EngineLogger::GetInstance().Add(::CalyxEngine::LogLevel::Error, ::CalyxEngine::LogCategory::Engine, (message), __FUNCTION__)
