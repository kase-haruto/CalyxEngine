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

/*-----------------------------------------------------------------------------------------
 * AssetRecord
 * - Asset Databaseが管理する一Asset分のメタ情報を保持するデータ構造
 * - GUID、種別、パス、更新日時、Preview、Import設定、Tagを管理する
 * - Asset本体とPreview Texture Resourceの所有は担当しない
 *---------------------------------------------------------------------------------------*/
/**
 * @brief AssetRecordに関するデータを保持する構造体です。
 */
struct AssetRecord {
	AssetGUID guid{};
	AssetType type = AssetType::Unknown;
	std::filesystem::path sourcePath;
	std::filesystem::file_time_type lastWrite{};

	ImTextureID previewTex = nullptr;

	std::unordered_map<std::string, std::string> importSettings;
	std::vector<std::string> tags;
};
