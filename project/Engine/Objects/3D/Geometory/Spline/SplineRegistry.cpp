#include "SplineRegistry.h"

#include "SplineJson.h"

#include <CalyxEngine/Project.h>

#include <unordered_map>

namespace {
	std::unordered_map<std::string, std::weak_ptr<SplineData>> g_cache;

	std::string ResolveSplinePath(const std::string& path) {
		return path.empty() ? std::string{} : Calyx::ResolveAssetPath(path).generic_string();
	}
}

namespace SplineRegistry {
	std::shared_ptr<SplineData> Create() {
		return std::make_shared<SplineData>();
	}

	std::shared_ptr<SplineData> Acquire(const std::string& path) {
		const std::string resolvedPath = ResolveSplinePath(path);
		if(resolvedPath.empty()) return Create();
		if(auto it = g_cache.find(resolvedPath); it != g_cache.end()) {
			if(auto sp = it->second.lock()) {
				return sp;
			}
		}
		auto sp = Create();
		g_cache[resolvedPath] = sp;
		return sp;
	}

	bool LoadInto(const std::string& path, const std::shared_ptr<SplineData>& data) {
		if(!data) return false;
		const std::string resolvedPath = ResolveSplinePath(path);
		SplineData temp;
		if(!SplineJson::Load(resolvedPath, temp)) return false;
		*data = std::move(temp);
		data->MarkDirty();
		if(!resolvedPath.empty()) {
			g_cache[resolvedPath] = data;
		}
		return true;
	}

	bool SaveFrom(const std::string& path, const std::shared_ptr<SplineData>& data) {
		if(!data) return false;
		const std::string resolvedPath = ResolveSplinePath(path);
		if(!resolvedPath.empty()) {
			g_cache[resolvedPath] = data;
		}
		return SplineJson::Save(resolvedPath, *data);
	}

	std::shared_ptr<SplineData> GetOrLoad(const std::string& path) {
		const std::string resolvedPath = ResolveSplinePath(path);
		if(resolvedPath.empty()) return Create();
		if(auto it = g_cache.find(resolvedPath); it != g_cache.end()) {
			if(auto sp = it->second.lock()) {
				return sp;
			}
		}

		auto sp = Create();
		LoadInto(resolvedPath, sp);
		g_cache[resolvedPath] = sp;
		return sp;
	}
}
