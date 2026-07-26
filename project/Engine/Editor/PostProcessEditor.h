#pragma once
#include <Engine/Editor/BaseEditor.h>

class PostProcessCollection;
class PostEffectGraph;

namespace CalyxEngine {
	/*-----------------------------------------------------------------------------------------
	 * PostProcessEditor
	 * - PostProcess CollectionのPass構成を編集するEditor専用クラス
	 * - Passの有効状態、種別、実行順を編集してGraphへ反映する
	 * - CollectionとGraphの所有権およびGPUリソース管理は担当しない
	 *---------------------------------------------------------------------------------------*/
	/**
	 * @brief PostProcessEditorの機能を提供するクラスです。
	 */
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
