#include "AssetDatabase.h"

#include <Engine/Assets/Texture/TextureManager.h>
#include <externals/nlohmann/json.hpp>

#include <fstream>
#include <iostream>

using json = nlohmann::json;

AssetDatabase* AssetDatabase::GetInstance() {
	static AssetDatabase inst;
	return &inst;
}

void AssetDatabase::Initialize(const std::filesystem::path& assetsRoot) {
	assetsRoot_ = std::filesystem::weakly_canonical(assetsRoot);
	if (!std::filesystem::exists(assetsRoot_)) {
		std::filesystem::create_directories(assetsRoot_);
	}
	Scan();
}

std::filesystem::path AssetDatabase::ToAbsoluteUnderRoot(const std::filesystem::path& absOrRel) const {
	if (absOrRel.is_absolute()) {
		return std::filesystem::weakly_canonical(absOrRel);
	}
	return std::filesystem::weakly_canonical(assetsRoot_ / absOrRel);
}

std::string AssetDatabase::NormalizePath(const std::filesystem::path& p) {
	auto canon = std::filesystem::weakly_canonical(p).generic_string();
	for (auto& c : canon) c = (char)std::tolower((unsigned char)c);
	return canon;
}

AssetType AssetDatabase::GuessTypeFromExtension(const std::string& extIn) {
	std::string ext = extIn;
	for (auto& c : ext) c = (char)std::tolower((unsigned char)c);

	if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") return AssetType::Texture;
	if (ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx")                   return AssetType::Model;
	if (ext == ".hlsl" || ext == ".fxc" || ext == ".cso")                                    return AssetType::Shader;
	if (ext == ".mat")                                                                        return AssetType::Material;
	if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")                                      return AssetType::Audio;
	return AssetType::Unknown;
}

AssetGUID AssetDatabase::LoadOrCreateMeta(const std::filesystem::path& absPath, AssetType type) {
	auto metaPath = absPath;
	metaPath += ".meta";

	AssetGUID guid = Guid::Empty();

	if (std::filesystem::exists(metaPath)) {
		try {
			std::ifstream ifs(metaPath);
			json j; ifs >> j;
			if (j.contains("guid")) {
				// Guid は JSON で string に自動シリアライズされる（あなたの to_json/from_json 実装）
				guid = j.at("guid").get<Guid>();
			}
		} catch (...) {
			// 壊れていたら新規生成へ
		}
	}

	if (!guid.isValid()) {
		guid = Guid::New();
		try {
			json j{
				{"guid", guid},             // ← Guid は string に自動変換
				{"type", (int)type}
			};
			std::ofstream ofs(metaPath);
			ofs << j.dump(2);
		} catch (...) {
			std::cerr << "[AssetDB] meta write failed: " << metaPath << std::endl;
		}
	}
	return guid;
}

void AssetDatabase::BuildPreview(AssetRecord& rec) {
	auto& tm = *TextureManager::GetInstance();
	try {
		if (rec.type == AssetType::Texture) {
			std::filesystem::path rel = std::filesystem::relative(rec.sourcePath, assetsRoot_);
			auto texHandle = tm.LoadTexture(rel.string()); // あなたの TM 仕様に合わせて調整
			rec.previewTex = (ImTextureID)texHandle.ptr;
		} else {
			auto icon = tm.LoadTexture("UI/Icons/asset_generic.png");
			rec.previewTex = (ImTextureID)icon.ptr;
		}
	} catch (...) {
		rec.previewTex = nullptr;
	}
}

void AssetDatabase::RebuildViewCache() {
	viewCache_.clear();
	viewCache_.reserve(records_.size());
	for (auto& [g, recPtr] : records_) {
		viewCache_.push_back(recPtr.get());
	}
}

const AssetRecord* AssetDatabase::Get(const AssetGUID& guid) const {
	auto it = records_.find(guid);
	if (it == records_.end()) return nullptr;
	return it->second.get();
}

const AssetRecord* AssetDatabase::FindByPath(const std::filesystem::path& p) const {
	auto norm = NormalizePath(ToAbsoluteUnderRoot(p));
	auto it = normPathToGuid_.find(norm);
	if (it == normPathToGuid_.end()) return nullptr;
	return Get(it->second);
}

const std::filesystem::path& AssetDatabase::GetRoot() const noexcept {
	return assetsRoot_;
}

AssetGUID AssetDatabase::RegisterOrUpdate(const std::filesystem::path& absOrRelPath, AssetType forceType) {
	auto abs = ToAbsoluteUnderRoot(absOrRelPath);
	if (!std::filesystem::exists(abs)) return Guid::Empty();

	AssetType type = forceType;
	if (type == AssetType::Unknown) type = GuessTypeFromExtension(abs.extension().string());
	if (type == AssetType::Unknown) return Guid::Empty();

	auto guid = LoadOrCreateMeta(abs, type);
	auto norm = NormalizePath(abs);
	auto ft = std::filesystem::last_write_time(abs);

	auto it = records_.find(guid);
	if (it == records_.end()) {
		auto rec = std::make_unique<AssetRecord>();
		rec->guid = guid;
		rec->type = type;
		rec->sourcePath = abs;
		rec->lastWrite = ft;
		BuildPreview(*rec);

		normPathToGuid_[norm] = guid;
		records_.emplace(guid, std::move(rec));
	} else {
		auto& r = *it->second;
		bool needPreview = (r.type != type) || (r.sourcePath != abs);
		r.type = type;
		r.sourcePath = abs;
		r.lastWrite = ft;
		normPathToGuid_[norm] = guid;
		if (needPreview) BuildPreview(r);
	}

	RebuildViewCache();
	return guid;
}

void AssetDatabase::Scan() {
	if (!std::filesystem::exists(assetsRoot_)) return;

	for (auto& entry : std::filesystem::recursive_directory_iterator(assetsRoot_)) {
		if (!entry.is_regular_file()) continue;
		const auto& abs = entry.path();
		if (abs.extension() == ".meta") continue;

		auto type = GuessTypeFromExtension(abs.extension().string());
		if (type == AssetType::Unknown) continue;

		RegisterOrUpdate(abs, type);
	}

	RebuildViewCache();
}
