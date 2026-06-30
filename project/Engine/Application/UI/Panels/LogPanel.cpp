#include "LogPanel.h"

// engine
#include <Engine/Foundation/Log/EngineLogger.h>

// imgui
#include <externals/imgui/imgui.h>

// c++
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <string_view>
#include <vector>

namespace {

	const char* ToString(CalyxEngine::LogLevel level) {
		switch(level) {
		case CalyxEngine::LogLevel::Trace: return "Trace";
		case CalyxEngine::LogLevel::Info: return "Info";
		case CalyxEngine::LogLevel::Warning: return "Warning";
		case CalyxEngine::LogLevel::Error: return "Error";
		default: return "Unknown";
		}
	}

	const char* ToString(CalyxEngine::LogCategory category) {
		switch(category) {
		case CalyxEngine::LogCategory::Engine: return "Engine";
		case CalyxEngine::LogCategory::Editor: return "Editor";
		case CalyxEngine::LogCategory::Game: return "Game";
		case CalyxEngine::LogCategory::Asset: return "Asset";
		case CalyxEngine::LogCategory::Rendering: return "Rendering";
		case CalyxEngine::LogCategory::Physics: return "Physics";
		case CalyxEngine::LogCategory::Command: return "Command";
		default: return "Unknown";
		}
	}

	ImVec4 GetLevelColor(CalyxEngine::LogLevel level) {
		switch(level) {
		case CalyxEngine::LogLevel::Trace: return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
		case CalyxEngine::LogLevel::Info: return ImVec4(0.88f, 0.88f, 0.88f, 1.0f);
		case CalyxEngine::LogLevel::Warning: return ImVec4(1.0f, 0.78f, 0.25f, 1.0f);
		case CalyxEngine::LogLevel::Error: return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
		default: return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

	std::array<char, 9> FormatTime(const std::chrono::system_clock::time_point& time) {
		const std::time_t rawTime = std::chrono::system_clock::to_time_t(time);
		std::tm localTime{};
		localtime_s(&localTime, &rawTime);

		std::array<char, 9> result{};
		std::snprintf(result.data(), result.size(), "%02d:%02d:%02d", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
		return result;
	}

	std::string ToLower(std::string_view text) {
		std::string result(text);
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return result;
	}

	bool MatchesSearch(const CalyxEngine::LogEntry& entry, const char* searchText) {
		if(!searchText || searchText[0] == '\0') return true;

		// 表示対象の全項目を連結し、大文字・小文字を区別せず検索する。
		const std::string searchable = ToLower(
			std::string(ToString(entry.level)) + " " +
			ToString(entry.category) + " " + entry.message + " " + entry.source);
		return searchable.find(ToLower(searchText)) != std::string::npos;
	}

} // namespace

namespace CalyxEngine {

	LogPanel::LogPanel()
		: IEngineUI("Log") {
	}

	void LogPanel::Render() {
		if(!IsShow()) return;

		bool isOpen = true;
		if(!ImGui::Begin(panelName_.c_str(), &isOpen)) {
			ImGui::End();
			if(!isOpen) SetShow(false);
			return;
		}

		// ログ出力とターミナルを同じウィンドウ内のタブで切り替える。
		if(ImGui::BeginTabBar("LogPanelTabs")) {
			if(ImGui::BeginTabItem("Log")) {

		// 上部ツールバーでログ消去、自動スクロール、レベル絞り込みを操作する。
		if(ImGui::Button("Clear")) {
			EngineLogger::GetInstance().Clear();
			selectedEntryId_ = 0;
			lastDisplayedEntryId_ = 0;
		}
		ImGui::SameLine();
		ImGui::Checkbox("Auto Scroll", &autoScroll_);
		ImGui::SameLine();
		ImGui::Checkbox("Trace", &showTrace_);
		ImGui::SameLine();
		ImGui::Checkbox("Info", &showInfo_);
		ImGui::SameLine();
		ImGui::Checkbox("Warning", &showWarning_);
		ImGui::SameLine();
		ImGui::Checkbox("Error", &showError_);

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##LogSearch", "Search logs...", searchBuffer_.data(), searchBuffer_.size());

		// ロガーから一度だけコピーし、このフレーム中は一貫した内容を描画する。
		const std::vector<LogEntry> entries = EngineLogger::GetInstance().GetEntries();
		const std::uint64_t newestEntryId = entries.empty() ? 0 : entries.back().id;
		const bool hasNewEntry = newestEntryId != 0 && newestEntryId != lastDisplayedEntryId_;

		auto isLevelVisible = [this](LogLevel level) {
			switch(level) {
			case LogLevel::Trace: return showTrace_;
			case LogLevel::Info: return showInfo_;
			case LogLevel::Warning: return showWarning_;
			case LogLevel::Error: return showError_;
			default: return false;
			}
		};

		const float tableHeight = std::max(120.0f, ImGui::GetContentRegionAvail().y - 92.0f);
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable |
			ImGuiTableFlags_ScrollY;

		if(ImGui::BeginTable("LogEntries", 5, tableFlags, ImVec2(0.0f, tableHeight))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 72.0f);
			ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 72.0f);
			ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 90.0f);
			ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 160.0f);
			ImGui::TableHeadersRow();

			for(const LogEntry& entry : entries) {
				if(!isLevelVisible(entry.level) || !MatchesSearch(entry, searchBuffer_.data())) continue;

				const auto timeText = FormatTime(entry.time);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				const ImVec4 rowColor = GetLevelColor(entry.level);
				ImGui::PushStyleColor(ImGuiCol_Text, rowColor);

				// 行全体を選択可能にし、詳細欄で完全な内容を確認できるようにする。
				const std::string selectableLabel = std::string(timeText.data()) + "##LogEntry" + std::to_string(entry.id);
				if(ImGui::Selectable(
					   selectableLabel.c_str(),
					   selectedEntryId_ == entry.id,
					   ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
					selectedEntryId_ = entry.id;
				}

				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(ToString(entry.level));
				ImGui::TableSetColumnIndex(2);
				ImGui::TextUnformatted(ToString(entry.category));
				ImGui::TableSetColumnIndex(3);
				ImGui::TextUnformatted(entry.message.c_str());
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(entry.source.empty() ? "-" : entry.source.c_str());
				ImGui::PopStyleColor();
			}

			// 新しいログが届いたフレームだけ、表示末尾へスクロールする。
			if(autoScroll_ && hasNewEntry) {
				ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndTable();
		}
		lastDisplayedEntryId_ = newestEntryId;

		// 選択中ログが保持上限やClearで消えた場合は、選択状態も解除する。
		const auto selectedIt = std::find_if(entries.begin(), entries.end(), [this](const LogEntry& entry) {
			return entry.id == selectedEntryId_;
		});
		if(selectedEntryId_ != 0 && selectedIt == entries.end()) {
			selectedEntryId_ = 0;
		}

		ImGui::SeparatorText("Details");
		if(selectedIt != entries.end()) {
			const auto timeText = FormatTime(selectedIt->time);
			ImGui::Text("[%s][%s][%s]", timeText.data(), ToString(selectedIt->level), ToString(selectedIt->category));
			ImGui::TextWrapped("%s", selectedIt->message.c_str());
			if(!selectedIt->source.empty()) {
				ImGui::Text("Source: %s", selectedIt->source.c_str());
			}
		} else {
			ImGui::TextDisabled("Select a log entry to view details.");
		}

				ImGui::EndTabItem();
			}

			if(ImGui::BeginTabItem("Terminal")) {
				terminalView_.RenderContents();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::End();
		if(!isOpen) SetShow(false);
	}

	const std::string& LogPanel::GetPanelName() const {
		return panelName_;
	}

} // namespace CalyxEngine
