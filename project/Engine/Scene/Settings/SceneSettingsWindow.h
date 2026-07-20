#pragma once

class SceneContext;

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * SceneSettingsWindow
	 * - 現在シーン固有の設定カテゴリを選択・編集するEditorウィンドウ。
	 * - カテゴリ列と詳細領域を分離し、Collision以外の項目を後から追加しやすくする。
	 *---------------------------------------------------------------------------------------*/
	class SceneSettingsWindow {
	public:
		void Open();
		void Render(SceneContext* context);

	private:
		enum class Category {
			Collision,
			Rendering,
		};

		static const char* GetCategoryLabel(Category category);
		void DrawCategoryDetails(SceneContext& context);

		Category selectedCategory_ = Category::Collision;
		bool show_ = false;
	};
}
