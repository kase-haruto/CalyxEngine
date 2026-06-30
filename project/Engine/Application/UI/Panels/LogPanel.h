#pragma once

// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Application/UI/Panels/TerminalPanel.h>

// c++
#include <array>
#include <cstdint>
#include <string>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * LogPanel
	 * - ログ出力とターミナルをタブで切り替える統合エディタパネル
	 * - ログ管理とコマンド解釈は、UIから分離した専用クラスへ委譲する
	 *---------------------------------------------------------------------------------------*/
	class LogPanel : public IEngineUI {
	public:
		LogPanel();
		~LogPanel() override = default;

		void Render() override;
		const std::string& GetPanelName() const override;
		void SetCommandContext(const ConsoleCommandContext& context) { terminalView_.SetCommandContext(context); }

	private:
		// ログレベル別の表示状態。
		bool showTrace_ = true;
		bool showInfo_ = true;
		bool showWarning_ = true;
		bool showError_ = true;
		bool autoScroll_ = true;

		// 検索文字列を固定長バッファで保持する。
		std::array<char, 128> searchBuffer_{};

		// IDで選択状態を保持し、最大件数による先頭削除後も選択ずれを防ぐ。
		std::uint64_t selectedEntryId_ = 0;
		std::uint64_t lastDisplayedEntryId_ = 0;

		// ターミナルの状態と描画処理を専用ビューへ委譲する。
		TerminalPanel terminalView_;
	};

} // namespace CalyxEngine
