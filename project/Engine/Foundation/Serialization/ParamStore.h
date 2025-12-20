#pragma once

#include "SerializableObject.h"

#include <string>
#include <filesystem>

namespace CalyxEngine {

/* =========================================================================
   ParamStore
   - JsonUtils/JsonFileIO を使って保存・読込する裏方
   - 外部（ゲームコード）から直接呼ばず、SerializableObject経由で使う想定
   ========================================================================= */
	class ParamStore {
	public:
		static bool Save(const SerializableObject& obj);
		static bool Load(SerializableObject& obj);

	private:
		static std::filesystem::path MakeFilePath(const ParamPath& path);
	};

} // namespace CalyxSerialization
