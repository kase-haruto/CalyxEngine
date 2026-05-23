#pragma once
// std
#include <filesystem>

namespace CalyxEngine {

	/*------------------------
	 * 変更されたアセットのタイプ
	 *-----------------------*/
	enum class AssetChangeType {
		Asset_Model,
		Asset_Texture,
		Asset_Scene,
		Asset_Sound,
		Asset_Prefab
	};

	/*------------------------
	 * 発生したイベントの種類
	 *-----------------------*/
	 enum class AssetDeltaKind {
	 	Created,	//< 作成
	 	Modified,	//< 変更
	 	Deleted,	//< 削除
	 	Renamed,	//< 名前変更
	 };

	/*------------------------
	 *	変更イベント
	 *-----------------------*/
	struct AssetChangeEvent {
		AssetChangeType assetType;          // Model/Texture...
		AssetDeltaKind  deltaKind;          // どう変わったか
		std::filesystem::path path;         // 新しい/現在のパス
		std::filesystem::path oldPath;      // Renamed時のみ
		std::filesystem::file_time_type timestamp;
	};

}