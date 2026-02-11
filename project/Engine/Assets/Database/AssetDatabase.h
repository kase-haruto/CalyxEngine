#pragma once
#include <unordered_map>
#include <filesystem>
#include <memory>

#include "../System/AssetRecord.h"
#include "../System/AssetType.h"
#include <Engine/Foundation/Utility/Guid/Guid.h> // Guid

/*-----------------------------------------------------------------------------------------
 * AssetDatabase
 * - アセットデータベースクラス
 * - プロジェクト内のアセットのスキャン、GUIDによる管理、検索を担当するシングルトン
 *---------------------------------------------------------------------------------------*/
class AssetDatabase {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
	 * \brief インスタンスを取得
	 * \return インスタンス
	 */
	static AssetDatabase* GetInstance();

	/**
	 * \brief 初期化
	 * \param assetsRoot アセットのルートディレクトリ
	 */
	void Initialize(const std::filesystem::path& assetsRoot);

	/**
	 * \brief アセットディレクトリをスキャンしてレコードを作成/更新
	 */
	void Scan();

	// 検索
	/**
	 * \brief GUIDからアセットレコードを取得
	 * \param guid アセットGUID
	 * \return レコードポインタ
	 */
	const AssetRecord* Get(const AssetGUID& guid) const;

	/**
	 * \brief パスからアセットレコードを検索
	 * \param p ファイルパス
	 * \return レコードポインタ
	 */
	const AssetRecord* FindByPath(const std::filesystem::path& p) const;

	/**
	 * \brief アセットルートディレクトリを取得
	 * \return ルートパス
	 */
	const std::filesystem::path& GetRoot() const noexcept;

	/**
	 * \brief パネル表示用のアセットリストを取得
	 * \return アセットレコードリスト
	 */
	const std::vector<AssetRecord*>& GetView() const { return viewCache_; }

	/**
	 * \brief 外部から直接アセットを登録または更新
	 * \param absOrRelPath ファイルパス
	 * \param type アセットタイプ
	 * \return 生成または更新されたGUID
	 */
	AssetGUID RegisterOrUpdate(const std::filesystem::path& absOrRelPath, AssetType type);

	/**
	 * \brief 拡張子からアセットタイプを推測
	 * \param ext 拡張子
	 * \return 推測されるアセットタイプ
	 */
	static AssetType GuessTypeFromExtension(const std::string& ext);

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	std::filesystem::path assetsRoot_; //< アセットルートの絶対パス
	std::unordered_map<AssetGUID, std::unique_ptr<AssetRecord>> records_; //< GUIDをキーとしたアセットレコード管理マップ
	std::unordered_map<std::string, AssetGUID> normPathToGuid_;           //< 正規化パスからGUIDへの変換マップ

	std::vector<AssetRecord*> viewCache_; //< 表示用レコードキャッシュ

	//===================================================================*/
	//                    private methods
	//===================================================================*/
	/**
	 * \brief パスを正規化
	 * \param p 正規化するパス
	 * \return 正規化済みパス文字列
	 */
	static std::string NormalizePath(const std::filesystem::path& p);

	/**
	 * \brief メタファイルをロードまたは新規作成
	 * \param absPath アセットの絶対パス
	 * \param type アセットタイプ
	 * \return GUID
	 */
	AssetGUID LoadOrCreateMeta(const std::filesystem::path& absPath, AssetType type);

	/**
	 * \brief プレビューデータを構築
	 * \param rec 構築対象のレコード
	 */
	void BuildPreview(AssetRecord& rec);

	/**
	 * \brief 表示用キャッシュを再構築
	 */
	void RebuildViewCache();

	/**
	 * \brief ルート下の絶対パスへ変換
	 * \param absOrRel 絶対または相対パス
	 * \return 絶対パス
	 */
	std::filesystem::path ToAbsoluteUnderRoot(const std::filesystem::path& absOrRel) const;
};
