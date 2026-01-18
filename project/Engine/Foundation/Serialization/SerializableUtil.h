#pragma once

#include <string>
#include <vector>

namespace CalyxEngine {
	// 前方宣言
	struct SerializableField;
	struct VariableCategoryNode;

	class SerializableObject;

	std::vector<std::string> SplitCategory(const std::string& s);

	void BuildCategoryTree(
		VariableCategoryNode& root,
		const std::vector<SerializableField>& fields);

	bool DrawField(const SerializableField& f);

	bool DrawCategoryNode(const VariableCategoryNode& node, SerializableObject* owner = nullptr);
}