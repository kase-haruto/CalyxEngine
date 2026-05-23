#pragma once
#include <Engine/Foundation/Math/Vector2.h>
#include <externals/imgui/imgui.h>
#include <algorithm>
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
		static void AddMessage(const std::string& title, const std::string& body, const ImVec4& color = ImVec4(1, 1, 1, 1)) {
			GetInstance().messages_.push_back({title, body, color});
		}

		// ビューポート左上基準の座標へ、寿命付きのポップテキストを追加
		static void AddPopupText(const CalyxEngine::Vector2& position,
								 const ImVec4&			   color,
								 const std::string&		   text,
								 float					   lifetime = 2.0f,
								 float					   rise = 24.0f) {
			if(text.empty()) return;
			GetInstance().popupTexts_.push_back({position, color, text, lifetime, 0.0f, rise});
		}

		// メッセージの取得
		static const std::vector<Message>& GetMessages() {
			return GetInstance().messages_;
		}

		static const std::vector<PopupText>& GetPopupTexts() {
			return GetInstance().popupTexts_;
		}

		static void SetFatalAssert(const FatalAssert& fatal) {
			auto& instance = GetInstance();
			if(instance.hasFatalAssert_) return;
			instance.fatalAssert_ = fatal;
			instance.hasFatalAssert_ = true;
			instance.breakRequested_ = false;
		}

		static bool HasFatalAssert() {
			return GetInstance().hasFatalAssert_;
		}

		static const FatalAssert& GetFatalAssert() {
			return GetInstance().fatalAssert_;
		}

		static void RequestBreak() {
			GetInstance().breakRequested_ = true;
		}

		static bool ConsumeBreakRequest() {
			auto& instance = GetInstance();
			const bool requested = instance.breakRequested_;
			instance.breakRequested_ = false;
			if(requested) {
				instance.hasFatalAssert_ = false;
			}
			return requested;
		}

		static void UpdatePopupTexts(float dt) {
			auto& popups = GetInstance().popupTexts_;
			for(auto& popup : popups) {
				popup.age += dt;
			}

			popups.erase(
				std::remove_if(
					popups.begin(),
					popups.end(),
					[](const PopupText& popup) {
						return popup.age >= popup.lifetime;
					}),
				popups.end());
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
		std::vector<PopupText> popupTexts_;
		FatalAssert			   fatalAssert_;
		bool				   hasFatalAssert_ = false;
		bool				   breakRequested_ = false;
	};

} // namespace CalyxEngine
