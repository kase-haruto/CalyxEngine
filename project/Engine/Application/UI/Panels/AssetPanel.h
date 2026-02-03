#pragma once
// engine
#include <Engine/Application/UI/EngineUI/IEngineUI.h>
#include <Engine/Assets/System/AssetDragPayload.h>
#include <Engine/Assets/System/AssetRecord.h>
#include <Engine/Assets/System/AssetType.h>
#include <Engine/Assets/Texture/TextureManager.h>


// c++
#include <externals/imgui/imgui.h>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <vector>


namespace CalyxEditor {

	/*-----------------------------------------------------------------------------------------
	 * AssetPanel
	 * - アセットブラウザパネルクラス
	 * - プロジェクト内アセットのツリー表示・サムネイル表示・ドラッグドロップを提供
	 *---------------------------------------------------------------------------------------*/
	class AssetPanel
		: public IEngineUI {
	private:
		// --- フォルダツリー ---
		struct DirNode {
			std::string										name;
			std::filesystem::path							absPath;
			std::map<std::string, std::unique_ptr<DirNode>> children;
			bool											open	= false;
			bool											scanned = false;
		};

	public:
		AssetPanel() : IEngineUI("Assets") {}
		~AssetPanel() override = default;
		void Initialize(const std::filesystem::path& assetsRoot);
		void Render() override;

		// Inspector 側で使える：期待タイプを指定したドロップターゲット
		static bool DrawAssetDropTarget(AssetType expect, Guid* inoutGuid, float height = 56.0f);

	private:
		// --- 描画 ---
		void DrawMenuBar();
		void DrawToolbar();
		void DrawLeftTree();
		void DrawRightView();

		void DrawFavorites();
		void DrawDirNode(DirNode* node);

		// --- ツリー構築 ---
		void EnsureFolderTreeBuilt();
		void RebuildFolderTree(); // 実ファイルシステムから構築
		void InsertPath(DirNode* root, const std::filesystem::path& absDir);

		// --- 補助 ---
		static bool		   IsUnder(const std::filesystem::path& file, const std::filesystem::path& folder); // 再帰的配下か
		static std::string FilenameNoExt(const std::filesystem::path& p);
		static std::string NormalizeLower(const std::filesystem::path& p);									   // 大文字小文字を無視した比較用
		static bool		   IsInFolder(const std::filesystem::path& file, const std::filesystem::path& folder); // 「同一階層のみ」
		static void		   ListSubdirectories(const std::filesystem::path& folder, std::vector<std::filesystem::path>& out);

	private:
		// --- 状態 ---
		std::filesystem::path assetsRootAbs_;
		std::filesystem::path currentFolderAbs_;
		bool				  showLeftTree_ = true;	  // One/Two Column 切替
		bool				  gridMode_		= true;	  // Grid / List 切替
		float				  thumbSize_	= 84.0f;  // サムネサイズ
		float				  leftWidth_	= 240.0f; // 左ツリー幅
		char				  search_[128]	= {};	  // ゼロ初期化

		enum class Scope {
			All,
			SelectedFolder
		};
		Scope scope_ = Scope::All; // 初期は All の方がデバッグしやすい

		std::optional<AssetType> typeFilter_; // Favorites から設定（All なら std::nullopt）

		// アイコン
		ImTextureID iconFolder_	 = nullptr;
		ImTextureID iconGeneric_ = nullptr;

		std::unique_ptr<DirNode> rootNode_;
		bool					 needsRebuildTree_ = true;

		// ---- View cache ----
		std::vector<std::filesystem::path> cacheSubDirs_;
		std::vector<const AssetRecord*>	   cacheFilesHere_;
		std::filesystem::path			   cacheFolder_;
		std::string						   cacheSearch_;
		std::optional<AssetType>		   cacheType_;
		Scope							   cacheScope_		= Scope::All;
		size_t							   cacheItemsCount_ = 0;
		bool							   cacheValid_		= false;
	};
} // namespace CalyxEditor