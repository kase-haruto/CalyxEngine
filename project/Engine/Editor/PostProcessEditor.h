#pragma once
#include <Engine/Editor/BaseEditor.h>

class PostProcessCollection;
class PostEffectGraph;

namespace CalyxEngine {
	/* ========================================================================
	/*		ポストプロセス編集ツール
	/* ===================================================================== */
	class PostProcessEditor
		: public BaseEditor {
	public:
		PostProcessEditor(const std::string& name);
		~PostProcessEditor() = default;

		void ShowImGuiInterface() override;
		void ApplyToGraph(PostEffectGraph* graph);

	private:
		PostProcessCollection* pCollection_	  = nullptr;
		const std::string	   directoryPath_ = "Resources/Json/PostEffect/";
	};

}
