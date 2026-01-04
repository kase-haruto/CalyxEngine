#pragma once

#include <filesystem>

class FileSystemHelper {
public:
    static bool CreateDirectoryPath(const std::string& path);
};