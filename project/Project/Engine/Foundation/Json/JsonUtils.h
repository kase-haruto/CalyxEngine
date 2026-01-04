#pragma once

#include <Engine/Foundation/Utility/FileSystem/FileScanner.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>
#include <Engine/Foundation/Json/JsonFileIO.h>

#include <vector>
#include <filesystem>

namespace JsonUtils {

	// 保存（ディレクトリがなければ作成）
	template<typename T>
	bool Save(const std::string& path, const T& data);

	// 読み込み
	template<typename T>
	bool Load(const std::string& path, T& outData);

	// 読み込み or 作成して保存（デフォルト値付き）
	template<typename T>
	bool LoadOrCreate(const std::string& path, T& outData, const T& defaultValue = T{});

	// ディレクトリ内の全 JSON ファイルを読み込む（フィルタ付き）
	template<typename T>
	std::vector<T> LoadAllFromDirectory(const std::string& dirPath, const std::string& ext = ".json");

}

template<typename T>
bool JsonUtils::Save(const std::string& path, const T& data) {
	std::filesystem::path fsPath(path);
	FileSystemHelper::CreateDirectoryPath(fsPath.parent_path().string());

	nlohmann::json j = data;
	return JsonFileIO::Write(path, j);
}

template<typename T>
bool JsonUtils::Load(const std::string& path, T& outData) {
	if (!FileScanner::Exists(path)) return false;

	nlohmann::json j;
	if (!JsonFileIO::Read(path, j)) return false;

	outData = j.get<T>();
	return true;
}
template<typename T>
bool JsonUtils::LoadOrCreate(const std::string& path, T& outData, const T& defaultValue) {
	if (!FileScanner::Exists(path)) {
		outData = defaultValue;
		return Save(path, defaultValue);
	}
	return Load(path, outData);
}

template<typename T>
std::vector<T> JsonUtils::LoadAllFromDirectory(const std::string& dirPath, const std::string& ext) {
	std::vector<T> results;
	auto files = FileScanner::ScanFiles(dirPath, ext);

	for (const auto& file : files) {
		T data;
		if (Load(file.string(), data)) {
			results.push_back(std::move(data));
		}
	}
	return results;
}