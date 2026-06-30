#include "EngineLogger.h"

// c++
#include <algorithm>
#include <chrono>

namespace CalyxEngine {

	EngineLogger& EngineLogger::GetInstance() {
		static EngineLogger instance;
		return instance;
	}

	void EngineLogger::Add(LogLevel level,
						   LogCategory category,
						   const std::string& message,
						   const std::string& source) {
		// 複数スレッドからのログ追加とUIからの取得を直列化する。
		std::lock_guard lock(mutex_);

		LogEntry entry;
		entry.id = nextEntryId_++;
		entry.level = level;
		entry.category = category;
		entry.message = message;
		entry.source = source;
		entry.time = std::chrono::system_clock::now();
		entries_.push_back(std::move(entry));

		// 追加後に保持上限を適用し、ログが無制限に増えることを防ぐ。
		TrimToMaxEntries();
		++revision_;
	}

	void EngineLogger::Trace(const std::string& message, const std::string& source) {
		Add(LogLevel::Trace, LogCategory::Engine, message, source);
	}

	void EngineLogger::Info(const std::string& message, const std::string& source) {
		Add(LogLevel::Info, LogCategory::Engine, message, source);
	}

	void EngineLogger::Warning(const std::string& message, const std::string& source) {
		Add(LogLevel::Warning, LogCategory::Engine, message, source);
	}

	void EngineLogger::Error(const std::string& message, const std::string& source) {
		Add(LogLevel::Error, LogCategory::Engine, message, source);
	}

	void EngineLogger::Clear() {
		std::lock_guard lock(mutex_);
		entries_.clear();
		++revision_;
	}

	std::vector<LogEntry> EngineLogger::GetEntries() const {
		std::lock_guard lock(mutex_);
		return entries_;
	}

	std::uint64_t EngineLogger::GetRevision() const {
		std::lock_guard lock(mutex_);
		return revision_;
	}

	void EngineLogger::SetMaxEntries(std::size_t maxEntries) {
		std::lock_guard lock(mutex_);

		// 0件指定でもサービスを停止させず、最低1件は保持する。
		maxEntries_ = std::max<std::size_t>(1, maxEntries);
		TrimToMaxEntries();
		++revision_;
	}

	std::size_t EngineLogger::GetMaxEntries() const {
		std::lock_guard lock(mutex_);
		return maxEntries_;
	}

	void EngineLogger::TrimToMaxEntries() {
		if(entries_.size() <= maxEntries_) return;

		const auto eraseCount = entries_.size() - maxEntries_;
		entries_.erase(entries_.begin(), entries_.begin() + static_cast<std::ptrdiff_t>(eraseCount));
	}

} // namespace CalyxEngine
