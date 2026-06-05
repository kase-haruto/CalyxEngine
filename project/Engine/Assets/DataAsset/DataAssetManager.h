#pragma once

#include "DataAsset.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>

namespace CalyxEngine {

	/**
	 * @brief データアセット（マテリアル、スプライトアニメーションなど）を一括管理するクラス
	 * @details メモリ上にロードされたアセットのキャッシュ管理、ファイルからのロード、新規アセット作成、保存を担当します。
	 */
	class DataAssetManager {
	public:
		/**
		 * @brief コンストラクタ
		 */
		DataAssetManager() = default;

		/**
		 * @brief デストラクタ
		 */
		~DataAssetManager() = default;

		/**
		 * @brief アセットを登録する
		 * @param asset 登録するデータアセットのスマートポインタ
		 */
		void RegisterAsset(const std::shared_ptr<DataAsset>& asset);

		/**
		 * @brief GUIDからアセットを取得する
		 * @param guid 対象アセットのGUID
		 * @return 見つかったアセットへのスマートポインタ。存在しない場合はnullptr
		 */
		std::shared_ptr<DataAsset> GetAsset(const Guid& guid) const;

		/**
		 * @brief 型を指定してアセットを取得する（内部で動的キャスト）
		 * @tparam T キャスト先のアセットクラス型（MaterialAssetなど）
		 * @param guid 対象アセットのGUID
		 * @return 指定した型にキャストされたスマートポインタ。存在しないかキャスト失敗した場合はnullptr
		 */
		template <typename T>
		std::shared_ptr<T> GetAsset(const Guid& guid) const {
			return std::dynamic_pointer_cast<T>(GetAsset(guid));
		}

		/**
		 * @brief 名前からアセットを取得する（主にデバッグ・初期化用）
		 * @param name 検索するアセット名
		 * @return アセットへのスマートポインタ。存在しない場合はnullptr
		 */
		std::shared_ptr<DataAsset> GetAssetByName(const std::string& name) const;

		/**
		 * @brief 登録されている全アセットのマップを取得
		 * @return GUIDをキーとしたアセットマップの不変参照
		 */
		const std::unordered_map<Guid, std::shared_ptr<DataAsset>>& GetAssets() const { return assets_; }

		/**
		 * @brief アセットを管理リストから除外する
		 * @param guid 除外対象アセットのGUID
		 */
		void UnregisterAsset(const Guid& guid);

		/**
		 * @brief ファイルからマテリアルアセットをロードする
		 * @param path JSONアセットファイルのパス
		 * @param guid アセットのGUID
		 * @return ロードまたは新規作成されたMaterialAssetのスマートポインタ
		 */
		std::shared_ptr<class MaterialAsset> LoadMaterialAsset(const std::filesystem::path& path, const Guid& guid);

		/**
		 * @brief ファイルからスプライトアニメーションアセットをロードする
		 * @param path JSONアセットファイルのパス
		 * @param guid アセットのGUID
		 * @return ロードまたは新規作成されたSpriteAnimationAssetのスマートポインタ
		 */
		std::shared_ptr<class SpriteAnimationAsset> LoadSpriteAnimationAsset(const std::filesystem::path& path, const Guid& guid);

		/**
		 * @brief データアセットの内容をJSONファイルに保存する
		 * @param asset 保存対象のデータアセット
		 * @param path 保存先のファイルパス
		 * @return 保存に成功した場合はtrue、失敗した場合はfalse
		 */
		bool SaveAsset(const DataAsset& asset, const std::filesystem::path& path) const;

		/**
		 * @brief 新規マテリアルアセットを作成してファイル保存し、管理に登録する
		 * @param path 作成先のファイルパス
		 * @param guid 指定するGUID（無効な場合は自動発番）
		 * @param name アセット名（空の場合はファイル名を使用）
		 * @return 作成されたマテリアルアセットのスマートポインタ
		 */
		std::shared_ptr<class MaterialAsset> CreateMaterialAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name);

		/**
		 * @brief 新規スプライトアニメーションアセットを作成してファイル保存し、管理に登録する
		 * @param path 作成先のファイルパス
		 * @param guid 指定するGUID（無効な場合は自動発番）
		 * @param name アセット名（空の場合はファイル名を使用）
		 * @return 作成されたスプライトアニメーションアセットのスマートポインタ
		 */
		std::shared_ptr<class SpriteAnimationAsset> CreateSpriteAnimationAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name);

	private:
		std::unordered_map<Guid, std::shared_ptr<DataAsset>> assets_; //< GUIDをキーとしたデータアセットキャッシュ
	};

}
