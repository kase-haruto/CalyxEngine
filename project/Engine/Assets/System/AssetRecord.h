#pragma once

// engine
#include "AssetType.h"
#include <Engine/Foundation/Utility/Guid/Guid.h>

// c++
#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>

// externals
#include <externals/imgui/imgui.h>

using AssetGUID = Guid;

/* ========================================================================
/*		あっせと単位
/* ===================================================================== */
struct AssetRecord {
	AssetGUID guid{};
	AssetType type = AssetType::Unknown;
	std::filesystem::path sourcePath;
	std::filesystem::file_time_type lastWrite{};

	ImTextureID previewTex = nullptr;

	std::unordered_map<std::string, std::string> importSettings;
	std::vector<std::string> tags;
};
