#include "AssetDatabase.h"

#include <Engine/Assets/Texture/TextureManager.h>
#include <externals/nlohmann/json.hpp>

#include <fstream>
#include <iostream>

using json = nlohmann::json;

/////////////////////////////////////////////////////////////////////////////////////////
//		インスタンス取得（シングルトン）
/////////////////////////////////////////////////////////////////////////////////////////
AssetDatabase* AssetDatabase::GetInstance() {
	static AssetDatabase inst;
	return &inst;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		初期化
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::Initialize(const std::filesystem::path& assetsRoot) {
	// アセットフォルダのルートパスを正規化して保持
	assetsRoot_ = std::filesystem::weakly_canonical(assetsRoot);

	// 存在しない場合は作成
	if(!std::filesystem::exists(assetsRoot_)) {
		std::filesystem::create_directories(assetsRoot_);
	}

	// フォルダ全体をスキャンして登録
	Scan();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ルート下の絶対パスを取得
/////////////////////////////////////////////////////////////////////////////////////////
std::filesystem::path AssetDatabase::ToAbsoluteUnderRoot(const std::filesystem::path& absOrRel) const {
	// 引数が絶対パスならそのまま正規化して返す
	if(absOrRel.is_absolute()) {
		return std::filesystem::weakly_canonical(absOrRel);
	}
	// 相対パスならルートと結合して正規化
	return std::filesystem::weakly_canonical(assetsRoot_ / absOrRel);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		パス正規化（大文字小文字無視・スラッシュ統一）
/////////////////////////////////////////////////////////////////////////////////////////
std::string AssetDatabase::NormalizePath(const std::filesystem::path& p) {
	auto canon = std::filesystem::weakly_canonical(p).generic_string();
	for(auto& c : canon) c = (char)std::tolower((unsigned char)c);
	return canon;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		拡張子からアセット種別を推定
/////////////////////////////////////////////////////////////////////////////////////////
AssetType AssetDatabase::GuessTypeFromExtension(const std::string& extIn) {
	std::string ext = extIn;
	for(auto& c : ext) c = (char)std::tolower((unsigned char)c);

	if(ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds" || ext == ".tga") return AssetType::Texture;
	if(ext == ".obj" || ext == ".gltf" || ext == ".glb" || ext == ".fbx") return AssetType::Model;
	if(ext == ".hlsl" || ext == ".fxc" || ext == ".cso") return AssetType::Shader;
	if(ext == ".mat") return AssetType::Material;
	if(ext == ".wav" || ext == ".mp3" || ext == ".ogg") return AssetType::Audio;
	return AssetType::Unknown;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		メタファイルをロードまたは新規作成
/////////////////////////////////////////////////////////////////////////////////////////
AssetGUID AssetDatabase::LoadOrCreateMeta(const std::filesystem::path& absPath, AssetType type) {
	auto metaPath = absPath;
	metaPath += ".meta";

	AssetGUID guid = Guid::Empty();

	// 既存の .meta ファイルを読み込み
	if(std::filesystem::exists(metaPath)) {
		try {
			std::ifstream ifs(metaPath);
			json		  j;
			ifs >> j;
			if(j.contains("guid")) {
				// Guid は string に自動シリアライズされる（to_json/from_json 実装依存）
				guid = j.at("guid").get<Guid>();
			}
		} catch(...) {
			// 壊れていた場合は新規生成へ移行
		}
	}

	// GUID が無効なら新規作成
	if(!guid.isValid()) {
		guid = Guid::New();
		try {
			json j{
				{"guid", guid}, // Guid は string に自動変換
				{"type", (int)type}};
			std::ofstream ofs(metaPath);
			ofs << j.dump(2); // インデント2で保存
		} catch(...) {
			std::cerr << "[AssetDB] meta write failed: " << metaPath << std::endl;
		}
	}
	return guid;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アセットのプレビュー用テクスチャを構築
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::BuildPreview(AssetRecord& rec) {
	auto& tm = *TextureManager::GetInstance();
	try {
		if(rec.type == AssetType::Texture) {
			// モデルフォルダ内のテクスチャはプレビュー生成をスキップ（ロード失敗の可能性があるため）
			std::filesystem::path rel	 = std::filesystem::relative(rec.sourcePath, assetsRoot_);
			std::string			  relStr = rel.generic_string();
			if(relStr.find("models/") == 0 || relStr.find("Models/") == 0) {
				// モデル内のテクスチャはアイコンで代用
				auto icon	   = tm.LoadTexture("UI/Tool/AssetPanel/generic.dds");
				rec.previewTex = (ImTextureID)icon.ptr;
			} else {
				// テクスチャタイプの場合は実際の画像をプレビューとして使用
				auto texHandle = tm.LoadTexture(rel.string());
				rec.previewTex = (ImTextureID)texHandle.ptr;
			}
		} else {
			// その他のタイプは共通アイコンを使用
			auto icon	   = tm.LoadTexture("UI/Tool/AssetPanel/generic.dds");
			rec.previewTex = (ImTextureID)icon.ptr;
		}
	} catch(...) {
		rec.previewTex = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		ビューキャッシュ再構築（ImGuiなどで一覧表示用）
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::RebuildViewCache() {
	viewCache_.clear();
	viewCache_.reserve(records_.size());
	for(auto& [g, recPtr] : records_) {
		viewCache_.push_back(recPtr.get());
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
//		GUID からアセット情報を取得
/////////////////////////////////////////////////////////////////////////////////////////
const AssetRecord* AssetDatabase::Get(const AssetGUID& guid) const {
	auto it = records_.find(guid);
	if(it == records_.end()) return nullptr;
	return it->second.get();
}

/////////////////////////////////////////////////////////////////////////////////////////
//		パスからアセットを検索
/////////////////////////////////////////////////////////////////////////////////////////
const AssetRecord* AssetDatabase::FindByPath(const std::filesystem::path& p) const {
	auto norm = NormalizePath(ToAbsoluteUnderRoot(p));
	auto it	  = normPathToGuid_.find(norm);
	if(it == normPathToGuid_.end()) return nullptr;
	return Get(it->second);
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アセットルートを取得
/////////////////////////////////////////////////////////////////////////////////////////
const std::filesystem::path& AssetDatabase::GetRoot() const noexcept {
	return assetsRoot_;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		アセット登録または更新
/////////////////////////////////////////////////////////////////////////////////////////
AssetGUID AssetDatabase::RegisterOrUpdate(const std::filesystem::path& absOrRelPath, AssetType forceType) {
	auto abs = ToAbsoluteUnderRoot(absOrRelPath);
	if(!std::filesystem::exists(abs)) return Guid::Empty();

	// 種別を自動判定（明示指定が Unknown の場合）
	AssetType type = forceType;
	if(type == AssetType::Unknown) type = GuessTypeFromExtension(abs.extension().string());
	if(type == AssetType::Unknown) return Guid::Empty();

	// メタファイル読み込みまたは生成
	auto guid = LoadOrCreateMeta(abs, type);
	auto norm = NormalizePath(abs);
	auto ft	  = std::filesystem::last_write_time(abs);

	// 既存登録があるかチェック
	auto it = records_.find(guid);
	if(it == records_.end()) {
		// 新規登録
		auto rec		= std::make_unique<AssetRecord>();
		rec->guid		= guid;
		rec->type		= type;
		rec->sourcePath = abs;
		rec->lastWrite	= ft;
		BuildPreview(*rec);

		normPathToGuid_[norm] = guid;
		records_.emplace(guid, std::move(rec));
	} else {
		// 既存の更新
		auto& r				  = *it->second;
		bool  needPreview	  = (r.type != type) || (r.sourcePath != abs);
		r.type				  = type;
		r.sourcePath		  = abs;
		r.lastWrite			  = ft;
		normPathToGuid_[norm] = guid;
		if(needPreview) BuildPreview(r);
	}

	RebuildViewCache();
	return guid;
}

/////////////////////////////////////////////////////////////////////////////////////////
//		フォルダ全体をスキャンしアセット登録
/////////////////////////////////////////////////////////////////////////////////////////
void AssetDatabase::Scan() {
	if(!std::filesystem::exists(assetsRoot_)) return;

	for(auto& entry : std::filesystem::recursive_directory_iterator(assetsRoot_)) {
		if(!entry.is_regular_file()) continue;
		const auto& abs = entry.path();
		if(abs.extension() == ".meta") continue; // メタファイルはスキップ

		auto type = GuessTypeFromExtension(abs.extension().string());
		if(type == AssetType::Unknown) continue;

		RegisterOrUpdate(abs, type);
	}

	RebuildViewCache();
}
