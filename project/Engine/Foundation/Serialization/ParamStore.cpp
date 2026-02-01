#include "ParamStore.h"

#include <Engine/Foundation/Json/JsonUtils.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>

#include <algorithm>

namespace CalyxEngine {

	namespace {
		static constexpr const char* kRootDir = "Resources/Params/";

		std::string ToString(ParamDomain d) {
			switch (d) {
			case ParamDomain::Game:   return "Game";
			case ParamDomain::Engine: return "Engine";
			case ParamDomain::Editor: return "Editor";
			}
			return "Unknown";
		}
	}
	
	std::filesystem::path ParamStore::MakeFilePath(const ParamPath& path) {
		std::filesystem::path p = kRootDir;

		// Game / Engine / Editor
		p /= ToString(path.domain);

		// サブディレクトリ
		if (path.subDirectory.has_value()) {
			p /= path.subDirectory.value();
		}

		// name フォルダ
		p /= path.name;

		// name.json
		p /= path.name;
		p += ".json";

		return p;
	}

	
	bool ParamStore::Save(const SerializableObject& obj) {
		const auto filePath = MakeFilePath(obj.GetParamPath());

		FileSystemHelper::CreateDirectoryPath(filePath.parent_path().string());

		Json j;
		j["fields"] = Json::object();

		for (const auto& f : obj.Fields()) {
			Json v;
			WriteValue(v, f.ptr);
			j["fields"][f.key] = v;
		}

		return CalyxUtil::JsonUtils::Save(filePath.string(), j);
	}

	bool ParamStore::Load(SerializableObject& obj) {
		const auto filePath = MakeFilePath(obj.GetParamPath());

		Json j;
		if (!CalyxUtil::JsonUtils::Load(filePath.string(), j)) {
			return false;
		}

		if (!j.contains("fields") || !j["fields"].is_object()) {
			return false;
		}

		for (auto& f : obj.FieldsMutable()) {
			if (!j["fields"].contains(f.key)) continue;
			ReadValue(j["fields"][f.key], f.ptr);
		}

		return true;
	}


} // namespace CalyxSerialization