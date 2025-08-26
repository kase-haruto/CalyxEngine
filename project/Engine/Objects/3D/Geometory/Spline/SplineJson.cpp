#include "SplineJson.h"
#include <externals/nlohmann/json.hpp>
#include <fstream>

using nlohmann::json;

static void to_json(json& j, const SplinePoint& p) {
	j = json{ {"x", p.pos.x}, {"y", p.pos.y}, {"z", p.pos.z} };
}
static void from_json(const json& j, SplinePoint& p) {
	j.at("x").get_to(p.pos.x);
	j.at("y").get_to(p.pos.y);
	j.at("z").get_to(p.pos.z);
}

namespace SplineJson {
	bool Save(const std::string& path, const SplineData& s) {
		json j;
		j["closed"] = s.closed;
		j["points"] = s.points;
		std::ofstream ofs(path, std::ios::binary);
		if (!ofs) return false;
		ofs << j.dump(2);
		return true;
	}
	bool Load(const std::string& path, SplineData& s) {
		std::ifstream ifs(path, std::ios::binary);
		if (!ifs) return false;
		json j; ifs >> j;
		s.closed = j.value("closed", false);
		s.points = j.value("points", std::vector<SplinePoint>{});
		return true;
	}
}