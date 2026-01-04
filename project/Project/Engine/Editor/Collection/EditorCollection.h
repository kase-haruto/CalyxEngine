#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */
#include <Engine/Editor/BaseEditor.h>
#include <unordered_map>
#include <memory>

class EditorCollection{
public:
	enum class EditorType{
		PostProcess,
		Ui,
	};
public:
	//===================================================================*/
	//		public functions
	//===================================================================*/
	EditorCollection() = default;
	~EditorCollection() = default;

	void InitializeEditors();
	void UpdateEditors();
	// accessor =========================================================//
	BaseEditor* GetEditor(EditorType editorType);
private:
	//===================================================================*/
	//		private functions
	//===================================================================*/

private:
	//===================================================================*/
	//		private variables
	//===================================================================*/
	std::unordered_map<EditorType, std::unique_ptr<BaseEditor>> editors_;
};

