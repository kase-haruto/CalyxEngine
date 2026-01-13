#pragma once

#include "Engine/Foundation/Math/Vector2.h"
#include "Engine/Foundation/Math/Vector4.h"
#include "SerializableField.h"
#include "SerializableFieldBuilder.h"

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

		virtual ParamPath GetParamPath() const { return {ParamDomain::Game,"Default"}; }

		// --- 各オブジェクトから呼ぶAPI ---
		bool SaveParams() const;
		bool LoadParams();

		void         SaveAndLoadButtonGui();

		std::vector<SerializableField>&       FieldsMutable() { return fields_; }
		const std::vector<SerializableField>& Fields() const { return fields_; }

	private:
		void ShowInspector();

	protected:
		SerializableObject() = default;

		template <typename T>
		FieldBuilder AddField(const std::string& key,T& value) {
			fields_.push_back(SerializableField{
					key,
					ValuePtr{&value}
				});
			return FieldBuilder(fields_.back());
		}

	private:
		std::vector<SerializableField> fields_;
	};

} // namespace CalyxEngine