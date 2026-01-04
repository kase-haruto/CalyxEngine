#include "JsonFileIO.h"

#include <fstream>

bool JsonFileIO::Write(const std::string& filePath, const nlohmann::json& jsonData){
	std::ofstream ofs(filePath);
	if (!ofs.is_open()) return false;
	ofs << jsonData.dump(4);
	return true;
}

bool JsonFileIO::Read(const std::string& filePath, nlohmann::json& jsonData){
	std::ifstream ifs(filePath);
	if (!ifs.is_open()) return false;

	try{
		ifs >> jsonData;
	} catch (...){
		return false;
	}

	return true;
}