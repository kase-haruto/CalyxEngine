#pragma once

/* ========================================================================
/* 		include space
/* ===================================================================== */

// c++
#include <string>
#include <externals/nlohmann/json.hpp>

class JsonFileIO{
public:
	//===================================================================*/
	//                   public functions
	//===================================================================*/
	JsonFileIO() = default;
	~JsonFileIO() = default;

	static bool Write(const std::string& filePath, const nlohmann::json& jsonData);
	static bool Read(const std::string& filePath, nlohmann::json& jsonData);
};

