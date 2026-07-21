#include "EffectAsset.h"

#include <CalyxEngine/Project.h>
#include <Engine/Foundation/Utility/FileSystem/FileSystemHelper.h>
#include <externals/nlohmann/json.hpp>

#include <fstream>

namespace CalyxEngine {
	namespace {
		bool ReadJson(const std::filesystem::path& path, nlohmann::json& out) {
			std::ifstream ifs(path);
			if(!ifs) return false;
			try {
				ifs >> out;
			} catch(...) {
				return false;
			}
			return true;
		}

		bool WriteJson(const std::filesystem::path& path, const nlohmann::json& j) {
			FileSystemHelper::CreateDirectoryPath(path.parent_path().string());
			std::ofstream ofs(path);
			if(!ofs) return false;
			ofs << j.dump(2);
			return true;
		}
	}

	EffectAsset::EffectAsset(EffectAssetData data)
		: data_(std::move(data)) {}

	bool EffectAsset::Load(const std::filesystem::path& path) {
		const std::string name = path.stem().string();
		const std::filesystem::path fullPath = Calyx::ResolveAssetPath(std::filesystem::path("Effects") / (name + ".effect"));
		nlohmann::json root;
		if(!ReadJson(fullPath, root)) return false;
		data_ = root.get<EffectAssetData>();
		if(data_.name.empty()) {
			data_.name = fullPath.stem().string();
		}
		return true;
	}

	bool EffectAsset::Save(const std::filesystem::path& path) const {
		nlohmann::json root = data_;
		return WriteJson(path, root);
	}

	EffectAsset EffectAsset::FromObjectConfig(const EffectObjectConfig& config) {
		EffectAssetData data{};
		data.name = config.name.empty() ? "Effect" : config.name;

		for(const auto& node : config.emitters) {
			EffectEmitterAssetData emitter{};
			emitter.name		   = node.name;
			emitter.transform	   = node.transform;
			emitter.isDrawEnable = node.isDrawEnable;
			emitter.isGpu		   = node.isGpu;
			emitter.emitter	   = nlohmann::json(node.emitter).get<EmitterConfig>();
			data.emitters.push_back(std::move(emitter));
		}

		return EffectAsset(std::move(data));
	}

	EffectObjectConfig EffectAsset::ToObjectConfig() const {
		EffectObjectConfig config{};
		config.name = data_.name;

		for(const auto& emitterData : data_.emitters) {
			EffectEmitterNodeConfig node{};
			node.name		  = emitterData.name;
			node.transform	  = emitterData.transform;
			node.isDrawEnable = emitterData.isDrawEnable;
			node.isGpu		  = emitterData.isGpu;
			node.emitter	  = nlohmann::json(emitterData.emitter).get<EmitterConfig>();
			config.emitters.push_back(std::move(node));
		}

		return config;
	}

} // namespace CalyxEngine
