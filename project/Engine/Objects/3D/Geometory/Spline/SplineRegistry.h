#pragma once

#include "SplineData.h"

#include <memory>
#include <string>

namespace SplineRegistry {
	std::shared_ptr<SplineData> Acquire(const std::string& path);
	std::shared_ptr<SplineData> Create();
	bool LoadInto(const std::string& path, const std::shared_ptr<SplineData>& data);
	bool SaveFrom(const std::string& path, const std::shared_ptr<SplineData>& data);
	std::shared_ptr<SplineData> GetOrLoad(const std::string& path);
}
