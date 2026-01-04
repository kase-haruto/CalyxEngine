#include "ConfigPathResolver.h"

std::string ConfigPathResolver::GetBaseDirectory() {
	return "Resources/Configs/Engine/Objects/";
}

std::string ConfigPathResolver::ResolvePath(const std::string& objectType, const std::string& objectName, const std::string& presetName) {

	//　ファイルパスを構築
	return GetBaseDirectory() + objectType + "/" + objectName + "/" + presetName + "/" + objectName + ".json";
}

