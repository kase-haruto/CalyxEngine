#pragma once
#include <unordered_map>
#include <filesystem>
#include <memory>

#include "../System/AssetRecord.h"
#include "../System/AssetType.h"
#include <Engine/Foundation/Utility/Guid/Guid.h> // Guid

/* ========================================================================
/*		テクスチャ/モデルなどのアセットを外部から取得できるようにする
/* ===================================================================== */
class AssetDatabase {
public:
	static AssetDatabase* GetInstance();

	void Initialize(const std::filesystem::path& assetsRoot);
	void Scan();

	// 検索
	const AssetRecord* Get(const AssetGUID& guid) const;
	const AssetRecord* FindByPath(const std::filesystem::path& p) const;
	const std::filesystem::path& GetRoot() const noexcept;
	// パネル用
	const std::vector<AssetRecord*>& GetView() const { return viewCache_; }

	// 外部から直接登録/更新
	AssetGUID RegisterOrUpdate(const std::filesystem::path& absOrRelPath, AssetType type);

	static AssetType GuessTypeFromExtension(const std::string& ext);

private:
	std::filesystem::path assetsRoot_;
	std::unordered_map<AssetGUID, std::unique_ptr<AssetRecord>> records_; // ← Guid をキー
	std::unordered_map<std::string, AssetGUID> normPathToGuid_;           // 正規化パス → Guid
	\
	std::vector<AssetRecord*> viewCache_;

	static std::string NormalizePath(const std::filesystem::path& p);
	AssetGUID LoadOrCreateMeta(const std::filesystem::path& absPath, AssetType type);
	void BuildPreview(AssetRecord& rec);
	void RebuildViewCache();
	std::filesystem::path ToAbsoluteUnderRoot(const std::filesystem::path& absOrRel) const;
};
