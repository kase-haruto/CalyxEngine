#pragma once
/* ========================================================================
/*		include space
/* ===================================================================== */
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Editor/BaseEditor.h>

// c++
#include <functional>
#include <string>
#include <vector>

/// <summary>
/// エディタパネル
/// </summary>
class EditorPanel
	: public IEngineUI {
	using OnEditorSelectedCallback = std::function<void(BaseEditor*)>;

public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	EditorPanel();
	~EditorPanel() = default;

	void Render() override;

	// 追加
	void AddEditor(const BaseEditor* editor);

	// 削除
	void RemoveEditor(const BaseEditor* editor);

	//--------- accessor -----------------------------------------------------
	const std::string& GetPanelName() const override;

	void SetOnEditorSelected(OnEditorSelectedCallback cb) { onEditorSelected_ = std::move(cb); }

private:
	//===================================================================*/
	//                   private variables
	//===================================================================*/
	std::vector<BaseEditor*> editors_; //< エディタのリスト
public:
	static int selectedEditorIndex; //< 選択されたエディタ
private:
	OnEditorSelectedCallback onEditorSelected_;
};
