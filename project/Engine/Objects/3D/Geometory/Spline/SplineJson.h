#pragma once
#include "SplineData.h"
#include <string>

namespace SplineJson {
	bool Save(const std::string& path, const SplineData& s);
	bool Load(const std::string& path, SplineData& s);
}
