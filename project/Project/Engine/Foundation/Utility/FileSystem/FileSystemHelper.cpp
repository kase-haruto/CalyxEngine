#include "FileSystemHelper.h"

bool FileSystemHelper::CreateDirectoryPath(const std::string& path) {
    std::error_code ec;
    return std::filesystem::create_directories(path, ec);
}