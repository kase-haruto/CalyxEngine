#pragma once
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Math/Vector2.h>
#include <externals/imgui/imgui.h>
#include <string>
#include <vector>

namespace CalyxEngine {

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

		struct PopupText {
			CalyxEngine::Vector2 position;
			ImVec4				  color;
			std::string			  text;
			float				  lifetime = 2.0f;
			float				  age	   = 0.0f;
			float				  rise	   = 24.0f;
		};

		struct FatalAssert {
			std::string expression;
			std::string message;
			std::string file;
			int			line = 0;
			ImVec4		color = ImVec4(1.0f, 0.18f, 0.18f, 1.0f);
		};

		// メッセージの追加
		static CALYX_API void AddMessage(const std::string& title, const std::string& body, const ImVec4& color = ImVec4(1, 1, 1, 1));

		// ビューポート左上基準の座標へ、寿命付きのポップテキストを追加
		static CALYX_API void AddPopupText(const CalyxEngine::Vector2& position,
										   const ImVec4&				 color,
										   const std::string&			 text,
										   float						 lifetime = 2.0f,
										   float						 rise = 24.0f);

		// メッセージの取得
		static CALYX_API const std::vector<Message>& GetMessages();

		static CALYX_API const std::vector<PopupText>& GetPopupTexts();

		static CALYX_API void SetFatalAssert(const FatalAssert& fatal);

		static CALYX_API bool HasFatalAssert();

		static CALYX_API const FatalAssert& GetFatalAssert();

		static CALYX_API void RequestBreak();

		static CALYX_API bool ConsumeBreakRequest();

		static CALYX_API void UpdatePopupTexts(float dt);

		// クリア（毎フレームの最初または最後に呼ぶ）
		static CALYX_API void Clear();

	private:
		DebugTextManager() = default;
		static CALYX_API DebugTextManager& GetInstance();

		std::vector<Message> messages_;
		std::vector<PopupText> popupTexts_;
		FatalAssert			   fatalAssert_;
		bool				   hasFatalAssert_ = false;
		bool				   breakRequested_ = false;
	};

} // namespace CalyxEngine
