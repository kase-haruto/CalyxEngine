#include "ConsoleCommandRegistry.h"

// c++
#include <algorithm>
#include <cctype>
#include <exception>

namespace CalyxEngine {

	ConsoleCommandRegistry& ConsoleCommandRegistry::GetInstance() {
		static ConsoleCommandRegistry instance;
		return instance;
	}

	bool ConsoleCommandRegistry::Register(ConsoleCommandDefinition definition) {
		definition.name = NormalizeName(definition.name);
		if(definition.name.empty() || !definition.callback) return false;

		// 同名登録は上書きし、Engine標準コマンドをGame側で明示的に差し替えられるようにする。
		std::lock_guard lock(mutex_);
		commands_[definition.name] = std::move(definition);
		return true;
	}

	bool ConsoleCommandRegistry::Unregister(const std::string& name) {
		std::lock_guard lock(mutex_);
		return commands_.erase(NormalizeName(name)) > 0;
	}

	ConsoleCommandResult ConsoleCommandRegistry::Execute(
		const std::string& name,
		const std::vector<std::string>& arguments,
		const ConsoleCommandContext& context) const {
		const auto definition = Find(name);
		if(!definition) {
			return {false, {{ConsoleOutputLevel::Warning, "Unknown command: " + name}}};
		}

		try {
			return definition->callback(context, arguments);
		} catch(const std::exception& exception) {
			return {false, {{ConsoleOutputLevel::Error, "Command failed: " + std::string(exception.what())}}};
		} catch(...) {
			return {false, {{ConsoleOutputLevel::Error, "Command failed with an unknown exception."}}};
		}
	}

	std::optional<ConsoleCommandDefinition> ConsoleCommandRegistry::Find(const std::string& name) const {
		std::lock_guard lock(mutex_);
		const auto it = commands_.find(NormalizeName(name));
		if(it == commands_.end()) return std::nullopt;
		return it->second;
	}

	std::vector<ConsoleCommandDefinition> ConsoleCommandRegistry::GetCommands() const {
		std::lock_guard lock(mutex_);
		std::vector<ConsoleCommandDefinition> result;
		result.reserve(commands_.size());
		for(const auto& [name, definition] : commands_) {
			result.push_back(definition);
		}
		std::sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.name < rhs.name;
		});
		return result;
	}

	std::string ConsoleCommandRegistry::NormalizeName(const std::string& name) {
		std::string result = name;
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return result;
	}

} // namespace CalyxEngine
