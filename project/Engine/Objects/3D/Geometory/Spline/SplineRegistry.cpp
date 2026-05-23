#include "SplineRegistry.h"

#include "SplineJson.h"

#include <unordered_map>

namespace {
	std::unordered_map<std::string, std::weak_ptr<SplineData>> g_cache;
}

namespace SplineRegistry {
	std::shared_ptr<SplineData> Create() {
		return std::make_shared<SplineData>();
	}

	std::shared_ptr<SplineData> Acquire(const std::string& path) {
		if(path.empty()) return Create();
		if(auto it = g_cache.find(path); it != g_cache.end()) {
			if(auto sp = it->second.lock()) {
				return sp;
			}
		}
		auto sp = Create();
		g_cache[path] = sp;
		return sp;
	}

	bool LoadInto(const std::string& path, const std::shared_ptr<SplineData>& data) {
		if(!data) return false;
		SplineData temp;
		if(!SplineJson::Load(path, temp)) return false;
		*data = std::move(temp);
		data->MarkDirty();
		if(!path.empty()) {
			g_cache[path] = data;
		}
		return true;
	}

	bool SaveFrom(const std::string& path, const std::shared_ptr<SplineData>& data) {
		if(!data) return false;
		if(!path.empty()) {
			g_cache[path] = data;
		}
		return SplineJson::Save(path, *data);
	}

	std::shared_ptr<SplineData> GetOrLoad(const std::string& path) {
		if(path.empty()) return Create();
		if(auto it = g_cache.find(path); it != g_cache.end()) {
			if(auto sp = it->second.lock()) {
				return sp;
			}
		}

		auto sp = Create();
		LoadInto(path, sp);
		g_cache[path] = sp;
		return sp;
	}
}
