#pragma once

#include <string>

namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * BaseEditor
	 * - エディタ基底クラス
	 * - ImGuiを使用したエディタUIの共通インターフェースを提供
	 *---------------------------------------------------------------------------------------*/
	class BaseEditor {
	public:
		BaseEditor(const std::string& name) : editorName_(name) {}
		BaseEditor()					  = default;
		virtual ~BaseEditor()			  = default;
		virtual void ShowImGuiInterface() = 0; // 純粋仮想関数

		const std::string& GetEditorName() const { return editorName_; }

	protected:
		std::string editorName_ = "Editor"; // エディタ名
	};
}

