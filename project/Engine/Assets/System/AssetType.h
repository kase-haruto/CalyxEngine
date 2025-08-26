#pragma once
#include <cstdint>

enum class AssetType 
	: uint8_t {
	Unknown = 0,
	Texture,
	Model,
	Shader,
	Material,
	Audio,
};
