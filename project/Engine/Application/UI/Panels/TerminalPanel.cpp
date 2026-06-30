#include "TerminalPanel.h"

// imgui
#include <externals/imgui/imgui.h>

// c++
#include <algorithm>
#include <cctype>
#include <cstddef>

namespace {

	ImVec4 GetTerminalLineColor(CalyxEngine::ConsoleOutputLevel level) {
		switch(level) {
		case CalyxEngine::ConsoleOutputLevel::Info: return ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
		case CalyxEngine::ConsoleOutputLevel::Warning: return ImVec4(1.0f, 0.78f, 0.25f, 1.0f);
		case CalyxEngine::ConsoleOutputLevel::Error: return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
		default: return ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
		}
	}

} // namespace

namespace CalyxEngine {

	TerminalPanel::TerminalPanel()
		: IEngineUI("Terminal") {
		// 初回表示時に、利用可能なコマンドの確認方法だけを案内する。
		AddLine(LineType::Info, "Type '/help' to list available commands. Press Tab to complete.");
	}

	void TerminalPanel::Render() {
		if(!IsShow()) return;

		bool isOpen = true;
		if(!ImGui::Begin(panelName_.c_str(), &isOpen)) {
			ImGui::End();
			if(!isOpen) SetShow(false);
			return;
		}

		RenderContents();

		ImGui::End();
		if(!isOpen) SetShow(false);
	}

	void TerminalPanel::RenderContents() {
		// ターミナル履歴だけを消去し、EngineLoggerのログには影響させない。
		if(ImGui::Button("Clear")) {
			history_.clear();
		}
		ImGui::SameLine();
		ImGui::Checkbox("Auto Scroll", &autoScroll_);
		RefreshCommandSuggestions();

		// 出力領域を暗色背景にし、一般的なターミナルとして視認しやすくする。
		const std::size_t visibleSuggestionCount = (std::min)(commandSuggestions_.size(), kMaxVisibleSuggestions);
		const float suggestionHeight = visibleSuggestionCount > 0
			? ImGui::GetTextLineHeightWithSpacing() * static_cast<float>(visibleSuggestionCount) + ImGui::GetStyle().FramePadding.y * 2.0f
			: 0.0f;
		const float outputHeight = std::max(
			80.0f,
			ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() - suggestionHeight);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.035f, 0.035f, 0.04f, 1.0f));
		if(ImGui::BeginChild("TerminalOutput", ImVec2(0.0f, outputHeight), true, ImGuiWindowFlags_HorizontalScrollbar)) {
			for(const TerminalLine& line : history_) {
				ImVec4 color;
				switch(line.type) {
				case LineType::Command: color = ImVec4(0.40f, 0.80f, 1.0f, 1.0f); break;
				case LineType::Warning: color = GetTerminalLineColor(ConsoleOutputLevel::Warning); break;
				case LineType::Error: color = GetTerminalLineColor(ConsoleOutputLevel::Error); break;
				case LineType::Info:
				default: color = GetTerminalLineColor(ConsoleOutputLevel::Info); break;
				}
				ImGui::TextColored(color, "%s", line.text.c_str());
			}

			// コマンド実行後だけ末尾へ移動し、手動で過去を見ている間の移動を防ぐ。
			if(autoScroll_ && scrollToBottom_) {
				ImGui::SetScrollHereY(1.0f);
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
		scrollToBottom_ = false;
		if(suggestionHeight > 0.0f) DrawCommandSuggestions(suggestionHeight);

		// Enterでコマンドを実行し、続けて入力できるようフォーカスを戻す。
		if(focusCommandInput_) {
			ImGui::SetKeyboardFocusHere();
			focusCommandInput_ = false;
		}
		ImGui::SetNextItemWidth(-1.0f);
		const bool submitted = ImGui::InputTextWithHint(
			   "##TerminalCommandInput",
			   "/command... (Tab to complete)",
			   commandBuffer_.data(),
			   commandBuffer_.size(),
			   ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion,
			   InputTextCallback,
			   this);
		const bool inputActive = ImGui::IsItemActive();

		if(inputActive && !commandSuggestions_.empty()) {
			if(ImGui::IsKeyPressed(ImGuiKey_DownArrow, false)) {
				selectedSuggestionIndex_ = (selectedSuggestionIndex_ + 1) % static_cast<int>(commandSuggestions_.size());
				scrollSuggestionToSelection_ = true;
			}
			if(ImGui::IsKeyPressed(ImGuiKey_UpArrow, false)) {
				selectedSuggestionIndex_ =
					(selectedSuggestionIndex_ - 1 + static_cast<int>(commandSuggestions_.size())) % static_cast<int>(commandSuggestions_.size());
				scrollSuggestionToSelection_ = true;
			}
		}

		if(submitted) {
			if(!commandSuggestions_.empty() && !IsSelectedSuggestionExactMatch()) {
				ApplyCommandSuggestion(commandSuggestions_[selectedSuggestionIndex_].name);
			} else {
				ExecuteCommand();
			}
			ImGui::SetKeyboardFocusHere(-1);
		}

	}

	int TerminalPanel::InputTextCallback(ImGuiInputTextCallbackData* data) {
		if(!data || data->EventFlag != ImGuiInputTextFlags_CallbackCompletion) return 0;
		auto* panel = static_cast<TerminalPanel*>(data->UserData);
		if(panel) panel->CompleteCommand(data);
		return 0;
	}

	void TerminalPanel::CompleteCommand(ImGuiInputTextCallbackData* data) {
		if(!data) return;

		const std::string input(data->Buf, static_cast<std::size_t>(data->BufTextLen));
		const std::size_t firstCharacter = input.find_first_not_of(" \t");
		if(firstCharacter == std::string::npos) {
			data->InsertChars(0, "/");
		}

		const std::string current(data->Buf, static_cast<std::size_t>(data->BufTextLen));
		const std::size_t slashPosition = current.find_first_not_of(" \t");
		if(slashPosition == std::string::npos || current[slashPosition] != '/') {
			AddLine(LineType::Warning, "Tab completion requires a '/' command prefix.");
			scrollToBottom_ = true;
			return;
		}

		const std::size_t commandBegin = slashPosition + 1;
		const std::size_t commandEnd = current.find_first_of(" \t", commandBegin);
		const std::size_t tokenEnd = commandEnd == std::string::npos ? current.size() : commandEnd;
		if(static_cast<std::size_t>(data->CursorPos) > tokenEnd) return;

		const std::string prefix = current.substr(commandBegin, tokenEnd - commandBegin);
		const std::vector<ConsoleCommandDefinition> suggestions = commandDispatcher_.GetCommandSuggestions(prefix);
		if(suggestions.empty()) {
			AddLine(LineType::Warning, "No command matches: /" + prefix);
			scrollToBottom_ = true;
			return;
		}

		const int suggestionIndex = (std::clamp)(selectedSuggestionIndex_, 0, static_cast<int>(suggestions.size()) - 1);
		const std::string& completion = suggestions[suggestionIndex].name;
		data->DeleteChars(static_cast<int>(commandBegin), static_cast<int>(tokenEnd - commandBegin));
		data->InsertChars(static_cast<int>(commandBegin), completion.c_str());
		const std::string completedInput(data->Buf, static_cast<std::size_t>(data->BufTextLen));
		const std::size_t completedEnd = completedInput.find_first_of(" \t", commandBegin);
		if(completedEnd == std::string::npos) data->InsertChars(data->BufTextLen, " ");
		commandSuggestions_.clear();
	}

	void TerminalPanel::RefreshCommandSuggestions() {
		const std::string input = commandBuffer_.data();
		const std::size_t slashPosition = input.find_first_not_of(" \t");
		if(slashPosition == std::string::npos || input[slashPosition] != '/') {
			commandSuggestions_.clear();
			suggestionPrefix_.clear();
			return;
		}

		const std::size_t commandBegin = slashPosition + 1;
		if(input.find_first_of(" \t", commandBegin) != std::string::npos) {
			commandSuggestions_.clear();
			return;
		}

		const std::string prefix = input.substr(commandBegin);
		if(prefix != suggestionPrefix_) {
			suggestionPrefix_ = prefix;
			selectedSuggestionIndex_ = 0;
		}
		commandSuggestions_ = commandDispatcher_.GetCommandSuggestions(prefix);
		if(commandSuggestions_.empty()) selectedSuggestionIndex_ = 0;
		else selectedSuggestionIndex_ = (std::clamp)(selectedSuggestionIndex_, 0, static_cast<int>(commandSuggestions_.size()) - 1);
	}

	void TerminalPanel::DrawCommandSuggestions(float height) {
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.055f, 0.055f, 0.06f, 1.0f));
		if(ImGui::BeginChild("TerminalCommandSuggestions", ImVec2(0.0f, height), true)) {
			if(ImGui::BeginTable("TerminalCommandSuggestionTable", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Command", ImGuiTableColumnFlags_WidthFixed, 190.0f);
				ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
				for(std::size_t index = 0; index < commandSuggestions_.size(); ++index) {
					const auto& suggestion = commandSuggestions_[index];
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					const std::string label = "/" + suggestion.name + "##CommandSuggestion" + std::to_string(index);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.48f, 0.86f, 0.58f, 1.0f));
					if(ImGui::Selectable(
						   label.c_str(),
						   selectedSuggestionIndex_ == static_cast<int>(index),
						   ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap)) {
						selectedSuggestionIndex_ = static_cast<int>(index);
						ApplyCommandSuggestion(suggestion.name);
					}
					ImGui::PopStyleColor();
					if(ImGui::IsItemHovered()) selectedSuggestionIndex_ = static_cast<int>(index);
					if(scrollSuggestionToSelection_ && selectedSuggestionIndex_ == static_cast<int>(index)) {
						ImGui::SetScrollHereY(0.5f);
					}
					ImGui::TableSetColumnIndex(1);
					ImGui::TextDisabled("%s", suggestion.description.c_str());
				}
				ImGui::EndTable();
			}
			scrollSuggestionToSelection_ = false;
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

	void TerminalPanel::ApplyCommandSuggestion(const std::string& commandName) {
		const std::string completed = "/" + commandName + " ";
		commandBuffer_.fill('\0');
		std::copy_n(completed.c_str(), (std::min)(completed.size(), commandBuffer_.size() - 1), commandBuffer_.data());
		commandSuggestions_.clear();
		suggestionPrefix_.clear();
		focusCommandInput_ = true;
	}

	bool TerminalPanel::IsSelectedSuggestionExactMatch() const {
		if(commandSuggestions_.empty() || selectedSuggestionIndex_ < 0 ||
		   selectedSuggestionIndex_ >= static_cast<int>(commandSuggestions_.size())) return false;

		std::string input = commandBuffer_.data();
		const std::size_t slashPosition = input.find_first_not_of(" \t");
		if(slashPosition == std::string::npos || input[slashPosition] != '/') return false;
		std::string commandName = input.substr(slashPosition + 1);
		std::transform(commandName.begin(), commandName.end(), commandName.begin(), [](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
		return commandName == commandSuggestions_[selectedSuggestionIndex_].name;
	}

	const std::string& TerminalPanel::GetPanelName() const {
		return panelName_;
	}

	void TerminalPanel::ExecuteCommand() {
		const std::string command = commandBuffer_.data();
		commandBuffer_.fill('\0');
		commandSuggestions_.clear();
		suggestionPrefix_.clear();
		if(command.empty()) return;

		AddLine(LineType::Command, "> " + command);
		const ConsoleCommandResult result = commandDispatcher_.Execute(command, commandContext_);

		if(result.clearRequested) {
			history_.clear();
			scrollToBottom_ = false;
			return;
		}

		for(const ConsoleOutputLine& output : result.output) {
			LineType lineType = LineType::Info;
			switch(output.level) {
			case ConsoleOutputLevel::Warning: lineType = LineType::Warning; break;
			case ConsoleOutputLevel::Error: lineType = LineType::Error; break;
			case ConsoleOutputLevel::Info:
			default: lineType = LineType::Info; break;
			}
			AddLine(lineType, output.message);
		}
		scrollToBottom_ = true;
	}

	void TerminalPanel::AddLine(LineType type, const std::string& text) {
		history_.push_back({type, text});
		if(history_.size() > kMaxHistoryLines) {
			const std::size_t eraseCount = history_.size() - kMaxHistoryLines;
			history_.erase(history_.begin(), history_.begin() + static_cast<std::ptrdiff_t>(eraseCount));
		}
	}

} // namespace CalyxEngine
