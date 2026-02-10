#pragma once
#include <cstdint>

/*-----------------------------------------------------------------------------------------
 * AssetType
 * - アセットタイプ列挙型
 *---------------------------------------------------------------------------------------*/
enum class AssetType 
	: uint8_t {
	Unknown = 0,
	Texture,
	Model,
	Shader,
	Material,
	Audio,
};
