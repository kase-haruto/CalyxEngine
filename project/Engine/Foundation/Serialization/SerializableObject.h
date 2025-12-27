#pragma once

#include "Engine/Foundation/Math/Vector2.h"
#include "Engine/Foundation/Math/Vector4.h"
#include "SerializableField.h"

#include <string>
#include <type_traits>
#include <vector>

namespace CalyxEngine {

	enum class ParamDomain {
		Game,
		Engine,
		Editor,
	};

	struct ParamPath {
		ParamDomain domain;
		std::string name;
	};

	/* =========================================================================
	   SerializableObject
	   - 非テンプレート
	   - AddFieldで保存対象を宣言
	   - SaveParams/LoadParamsは各オブジェクトが呼ぶ
	   ========================================================================= */
	class SerializableObject {
	public:
		virtual ~SerializableObject() = default;

		virtual ParamPath GetParamPath() const = 0;

		// --- 各オブジェクトから呼ぶAPI ---
		bool SaveParams() const;
		bool LoadParams();

		std::vector<SerializableField>& FieldsMutable() { return fields_; }
		const std::vector<SerializableField>& Fields() const { return fields_; }

	protected:
		SerializableObject() = default;

		template<typename T>
		void AddField(const std::string& key, T& value) {
			static_assert(
				std::is_same_v<T, int32_t> ||
				std::is_same_v<T, float> ||
				std::is_same_v<T, bool> ||
				std::is_same_v<T, CalyxMath::Vector2> ||
				std::is_same_v<T, CalyxMath::Vector3>||
				std::is_same_v<T,CalyxMath::Vector4>,
				"AddField: Unsupported type. Add it to ValuePtr and Read/WriteValue."
			);
			fields_.push_back(SerializableField{ key, &value });
		}



	private:
		std::vector<SerializableField> fields_;
	};

} // namespace CalyxEngine
