#pragma once

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/System/Command/ConsoleCommandDispatcher.h>

// c++
#include <array>
#include <string>
#include <vector>

struct ImGuiInputTextCallbackData;

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * TerminalPanel
	 * - コマンド入力とコマンド応答だけを表示するエディタパネル
	 * - エンジンログの表示責務はLogPanelへ残し、両者を独立して利用可能にする
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief TerminalPanelの機能を提供するクラスです。
	 */
	class TerminalPanel : public IEngineUI {
	public:
		TerminalPanel();
		~TerminalPanel() override = default;

		void Render() override;
		// 親ウィンドウ内へターミナルUIだけを埋め込む。
		void RenderContents();
		const std::string& GetPanelName() const override;
		void SetCommandContext(const ConsoleCommandContext& context) { commandContext_ = context; }

	private:
		enum class LineType {
		Command,
		Info,
		Warning,
		Error
		};

		/**
		 * @brief TerminalLineに関するデータを保持する構造体です。
		 */
		struct TerminalLine {
			LineType type = LineType::Info;
			std::string text;
		};

		void ExecuteCommand();
		void AddLine(LineType type, const std::string& text);
		static int InputTextCallback(ImGuiInputTextCallbackData* data);
		void CompleteCommand(ImGuiInputTextCallbackData* data);
		void RefreshCommandSuggestions();
		void DrawCommandSuggestions(float height);
		void ApplyCommandSuggestion(const std::string& commandName);
		bool IsSelectedSuggestionExactMatch() const;

	private:
		std::vector<TerminalLine> history_;
		std::array<char, 256> commandBuffer_{};
		ConsoleCommandDispatcher commandDispatcher_;
		ConsoleCommandContext commandContext_;
		std::vector<ConsoleCommandDefinition> commandSuggestions_;
		std::string suggestionPrefix_;
		int selectedSuggestionIndex_ = 0;
		bool autoScroll_ = true;
		bool scrollToBottom_ = false;
		bool focusCommandInput_ = false;
		bool scrollSuggestionToSelection_ = false;
		static constexpr std::size_t kMaxHistoryLines = 500;
		static constexpr std::size_t kMaxVisibleSuggestions = 7;
	};

} // namespace CalyxEngine
