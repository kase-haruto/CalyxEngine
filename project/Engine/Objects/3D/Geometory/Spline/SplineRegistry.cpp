#include "SplineRegistry.h"

#include "SplineJson.h"

#include <CalyxEngine/Project.h>

#include <unordered_map>

namespace {
	std::unordered_map<std::string, std::weak_ptr<SplineData>> g_cache;

	std::string ResolveSplinePath(const std::string& path) {
		// Cache KeyをProject基準の正規Pathへ統一し、同一Assetの表記差による重複を防ぐ。
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
		// 生存中の共有Splineがあれば再利用し、Registry自体はLifetimeを延長しない。
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
		// Load失敗時に既存dataを壊さないよう、一時Objectへ読み込んでから置換する。
		SplineData temp;
		if(!SplineJson::Load(resolvedPath, temp)) return false;
		// 読込後はRevisionを更新し、参照中の変形Objectへ再構築を通知する。
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
		// 保存対象を同じCache Keyへ関連付け、直後の参照が同一Instanceを取得できるようにする。
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

		// Cache Miss時だけDiskから読込み、以降のObject間ではSplineを共有する。
		auto sp = Create();
		LoadInto(resolvedPath, sp);
		g_cache[resolvedPath] = sp;
		return sp;
	}
}
