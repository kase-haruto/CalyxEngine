#pragma once
#include <Engine/Editor/BaseEditor.h>

class PostProcessCollection;

class PostProcessEditor : public BaseEditor {
public:
	PostProcessEditor(const std::string& name);
	~PostProcessEditor() = default;

	void ShowImGuiInterface() override;
	void SetPostEffectCollection(PostProcessCollection* postProcessCollection);
	void ApplyToGraph(class PostEffectGraph* graph);

private:
	PostProcessCollection* pCollection_ = nullptr;
	const std::string directoryPath_ = "Resources/Json/PostEffect/";
};
