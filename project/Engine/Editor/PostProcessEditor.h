#pragma once
#include <Engine/Editor/BaseEditor.h>

class PostProcessCollection;

/// <summary>
/// ポストプロセスを編集
/// </summary>
class PostProcessEditor 
	: public BaseEditor {
public:
	PostProcessEditor(const std::string& name);
	~PostProcessEditor() = default;

	void ShowImGuiInterface() override;

	/// <summary>
	/// graphに適用
	/// </summary>
	/// <param name="graph"></param>
	void ApplyToGraph(class PostEffectGraph* graph);

private:
	PostProcessCollection* pCollection_ = nullptr;
	const std::string directoryPath_ = "Resources/Json/PostEffect/";
};
