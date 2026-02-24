#pragma once
#include <externals/imgui/imgui.h>
#include <string>
#include <vector>

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * DebugTextManager
	 * - デバッグテキスト管理クラス
	 * - 各フレームで表示したい情報を保持・提供する
	 *---------------------------------------------------------------------------------------*/
	class DebugTextManager {
	public:
		struct Message {
			std::string title;
			std::string body;
			ImVec4		color;
		};

		// メッセージの追加
		static void AddMessage(const std::string& title, const std::string& body, const ImVec4& color = ImVec4(1, 1, 1, 1)) {
			GetInstance().messages_.push_back({title, body, color});
		}

		// メッセージの取得
		static const std::vector<Message>& GetMessages() {
			return GetInstance().messages_;
		}

		// クリア（毎フレームの最初または最後に呼ぶ）
		static void Clear() {
			GetInstance().messages_.clear();
		}

	private:
		DebugTextManager() = default;
		static DebugTextManager& GetInstance() {
			static DebugTextManager instance;
			return instance;
		}

		std::vector<Message> messages_;
	};

} // namespace CalyxEditor
