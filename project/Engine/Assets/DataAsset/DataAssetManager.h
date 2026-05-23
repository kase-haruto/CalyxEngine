#pragma once

#include "DataAsset.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <filesystem>

namespace CalyxEngine {

	/**
	 * @brief データアセットを一括管理するクラス
	 */
	class DataAssetManager {
	public:
		DataAssetManager() = default;
		~DataAssetManager() = default;

		/**
		 * @brief アセットを登録する
		 */
		void RegisterAsset(const std::shared_ptr<DataAsset>& asset);

		/**
		 * @brief GUIDからアセットを取得する
		 */
		std::shared_ptr<DataAsset> GetAsset(const Guid& guid) const;

		/**
		 * @brief 型を指定してアセットを取得する（内部でキャスト）
		 */
		template <typename T>
		std::shared_ptr<T> GetAsset(const Guid& guid) const {
			return std::dynamic_pointer_cast<T>(GetAsset(guid));
		}

		/**
		 * @brief 名前からアセットを取得する（主にデバッグ・初期化用）
		 */
		std::shared_ptr<DataAsset> GetAssetByName(const std::string& name) const;

		/**
		 * @brief 全アセットのマップを取得
		 */
		
		const std::unordered_map<Guid, std::shared_ptr<DataAsset>>& GetAssets() const { return assets_; }

		/**
		 * @brief アセットを削除する
		 */
		void UnregisterAsset(const Guid& guid);

		std::shared_ptr<class MaterialAsset> LoadMaterialAsset(const std::filesystem::path& path, const Guid& guid);
		std::shared_ptr<class SpriteAnimationAsset> LoadSpriteAnimationAsset(const std::filesystem::path& path, const Guid& guid);
		bool SaveAsset(const DataAsset& asset, const std::filesystem::path& path) const;
		std::shared_ptr<class MaterialAsset> CreateMaterialAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name);
		std::shared_ptr<class SpriteAnimationAsset> CreateSpriteAnimationAsset(const std::filesystem::path& path, const Guid& guid, const std::string& name);

	private:
		std::unordered_map<Guid, std::shared_ptr<DataAsset>> assets_;
	};

}
