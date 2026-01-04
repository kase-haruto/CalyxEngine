#pragma once
/* ========================================================================
/* include space
/* ===================================================================== */

//c++
#include <string>
#include <vector>
#include <filesystem>

class FileScanner {	
public:
	//指定したディレクトリのファイルを検索
	static std::vector<std::filesystem::path> ScanFiles(const std::string& rootDir, const std::string& extensionFilter = "");
	//指定したディレクトリにファイルが存在するか確認
    static bool Exists(const std::string& filePath);
	//指定した拡張子のファイルが存在するか確認
    static bool HasExtension(const std::filesystem::path& path, const std::string& ext);
	//指定したディレクトリが存在するか確認
    static bool EnsureDirectoryExists(const std::string& dirPath);
	//ファイル名取得
	static std::string GetFileName(const std::filesystem::path& path);
};

