#include "DialogueAsset.h"

#include <externals/nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace CalyxEngine {
	namespace {
		nlohmann::json SerializeLine(const DialogueLine& line) {
			nlohmann::json json;
			json = {
				{"guid", line.guid_},
				{"speaker", line.speaker_},
				{"text", line.text_},
				{"charactersPerSecond", line.charactersPerSecond_},
				{"autoAdvance", line.autoAdvance_},
				{"autoAdvanceDelay", line.autoAdvanceDelay_}
			};
			return json;
		}

		DialogueLine DeserializeLine(const nlohmann::json& json) {
			DialogueLine line;
			line.guid_ = json.value("guid", Guid::New());
			if(!line.guid_.isValid()) line.guid_ = Guid::New();
			line.speaker_ = json.value("speaker", std::string{});
			line.text_ = json.value("text", std::string{});
			line.charactersPerSecond_ = (std::max)(0.01f, json.value("charactersPerSecond", 30.0f));
			line.autoAdvance_ = json.value("autoAdvance", false);
			line.autoAdvanceDelay_ = (std::max)(0.0f, json.value("autoAdvanceDelay", 0.0f));
			return line;
		}
	}

	DialogueLine* DialogueAsset::FindLine(const Guid& guid) noexcept {
		const auto it = std::ranges::find(lines_, guid, &DialogueLine::guid_);
		return it == lines_.end() ? nullptr : &*it;
	}

	const DialogueLine* DialogueAsset::FindLine(const Guid& guid) const noexcept {
		const auto it = std::ranges::find(lines_, guid, &DialogueLine::guid_);
		return it == lines_.end() ? nullptr : &*it;
	}

	DialogueLine& DialogueAsset::AddLine() {
		return lines_.emplace_back(DialogueLine{.guid_ = Guid::New()});
	}

	bool DialogueAsset::RemoveLine(const Guid& guid) {
		const auto it = std::ranges::find(lines_, guid, &DialogueLine::guid_);
		if(it == lines_.end()) return false;
		lines_.erase(it);
		return true;
	}

	bool DialogueAsset::MoveLine(const Guid& guid, int offset) {
		const auto it = std::ranges::find(lines_, guid, &DialogueLine::guid_);
		if(it == lines_.end()) return false;
		const auto index = static_cast<std::ptrdiff_t>(std::distance(lines_.begin(), it));
		const auto target = index + offset;
		if(target < 0 || target >= static_cast<std::ptrdiff_t>(lines_.size())) return false;
		std::iter_swap(lines_.begin() + index, lines_.begin() + target);
		return true;
	}

	bool DialogueAsset::Load(const std::filesystem::path& path) {
		try {
			std::ifstream stream(path);
			if(!stream) return false;
			nlohmann::json json;
			stream >> json;
			std::vector<DialogueLine> loaded;
			for(const auto& item : json.value("lines", nlohmann::json::array())) {
				loaded.push_back(DeserializeLine(item));
			}
			lines_ = std::move(loaded);
			return true;
		} catch(...) {
			return false;
		}
	}

	bool DialogueAsset::Save(const std::filesystem::path& path) const {
		try {
			if(!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());
			nlohmann::json serializedLines = nlohmann::json::array();
			for(const DialogueLine& line : lines_) serializedLines.push_back(SerializeLine(line));
			nlohmann::json json{{"version", 1}, {"lines", std::move(serializedLines)}};
			std::ofstream stream(path);
			if(!stream) return false;
			stream << json.dump(2);
			return static_cast<bool>(stream);
		} catch(...) {
			return false;
		}
	}
}
