#pragma once
#include <externals/nlohmann/json.hpp>

class IConfigurable {
public:
	virtual ~IConfigurable() = default;

	virtual void ApplyConfigFromJson(const nlohmann::json& j) = 0;
	virtual void ExtractConfigToJson(nlohmann::json& j)const = 0;
};