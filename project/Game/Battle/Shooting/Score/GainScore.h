#pragma once

// engine
#include <Engine/Foundation/Utility/Guid/Guid.h>

// stl
#include <string>
#include <cstdint>

struct GainScore{
	int16_t amount;
	Guid id;
	std::string reason;
	std::vector<std::string> tag;

};