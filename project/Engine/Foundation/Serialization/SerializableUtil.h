#pragma once

#include <string>
#include <vector>

namespace CalyxEngine {
	// 前方宣言
	struct SerializableField;
	struct VariableCategoryNode;

	std::vector<std::string> SplitCategory(const std::string& s);

	void BuildCategoryTree(
		VariableCategoryNode& root,
		const std::vector<SerializableField>& fields);

	void DrawField(const SerializableField& f);

	void DrawCategoryNode(const VariableCategoryNode& node);
}