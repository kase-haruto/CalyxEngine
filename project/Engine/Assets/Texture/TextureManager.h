#pragma once
#include <Engine/Assets/Texture/Texture.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Utility/Guid/Guid.h>

/* c++ */
#include <unordered_map>
#include <string>
#include <d3d12.h>
#include <wrl.h>
#include <filesystem>


class ImGuiManager;

/*-----------------------------------------------------------------------------------------
 * TextureManager
 * - テクスチャ管理クラス
 * - テクスチャのロード、SRVハンドルの管理、および寿命管理を行うシングルトン
 *---------------------------------------------------------------------------------------*/
/**
 * @brief TextureManagerの機能を提供するクラスです。
 */
class TextureManager {
public:
	//===================================================================*/
	//                   public methods
	//===================================================================*/
	/**
 	* \brief デフォルトコンストラクタ
 	*/
	TextureManager() = default;

	/**
	 * \brief 初期化
	 * \param imgui ImGuiマネージャー
	 */
	void Initialize(ImGuiManager* imgui);

	/**
	 * \brief 終了処理
	 */
	void Finalize();

	// 既存API（文字列キー）
	/**
	 * \brief テクスチャをロード
	 * \param filePath ファイルパス
	 * \return GPUハンドル
	 */
	CALYX_API D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const std::string& filePath);
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTextureLinear(const std::string& filePath);

	/**
	 * \brief SRVハンドルを取得
	 * \param textureName テクスチャ名（キー）
	 * \return GPUハンドル
	 */
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const std::string& textureName) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle(const std::string& textureName) const;
	bool WriteSrvTo(const std::string& textureName, D3D12_CPU_DESCRIPTOR_HANDLE destination) const;

	/**
	 * \brief ロード済みテクスチャリストを取得
	 * \return テクスチャマップ
	 */
	const std::unordered_map<std::string,Texture>& GetLoadedTextures() const;

	// ========= GUID API =========
	/**
	 * \brief GUIDを用いてテクスチャをロード
	 * \param guid アセットGUID
	 * \return GPUハンドル
	 */
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTexture(const Guid& guid);
	D3D12_GPU_DESCRIPTOR_HANDLE LoadTextureLinear(const Guid& guid);

	/**
	 * \brief GUIDを用いてSRVハンドルを取得
	 * \param guid アセットGUID
	 * \return GPUハンドル
	 */
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandle(const Guid& guid) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle(const Guid& guid) const;
	bool WriteSrvTo(const Guid& guid, D3D12_CPU_DESCRIPTOR_HANDLE destination) const;

	/**
	 * \brief 指定したGUIDのテクスチャがロードされているか
	 * \param guid アセットGUID
	 * \return ロードされているか
	 */
	bool HasTexture(const Guid& guid) const;

	/**
	 * \brief 環境マップテクスチャをセット
	 * \param filePath ファイルパス
	 */
	void SetEnvironmentTexture(const std::string& filePath);

	/**
	 * \brief 環境マップテクスチャのSRVハンドルを取得
	 * \return GPUハンドル
	 */
	D3D12_GPU_DESCRIPTOR_HANDLE GetEnvironmentTextureSrvHandle() const;

	/**
	 * \brief 非同期ロード開始
	 */
	void StartUpLoad();

private:
	//===================================================================*/
	//                    private methods
	//===================================================================*/


	/**
	 * \brief GUIDからアセットレコードを検索
	 * \param guid GUID
	 * \return レコードポインタ
	 */
	const struct AssetRecord* FindTextureRecord(const Guid& guid) const;

	/**
	 * \brief アセット相対パスへ変換
	 * \param abs 絶対パス
	 * \param root ルートパス
	 * \return 相対パス文字列
	 */
	static std::string ToAssetsRelative(const std::filesystem::path& abs,const std::filesystem::path& root);

private:
	//===================================================================*/
	//                    private member variables
	//===================================================================*/
	Microsoft::WRL::ComPtr<ID3D12Device> device_;                      //< デバイス
	ImGuiManager*                        imgui_             = nullptr; //< ImGuiマネージャー
	UINT                                 descriptorSizeSrv_ = 0;       //< SRVディスクリプタサイズ

	std::unordered_map<std::string,Texture> textures_;  //< ロード済みテクスチャマップ
	std::unordered_map<Guid,std::string>    guidToKey_; //< GUIDからマップキーへの変換マップ

	std::string environmentTextureName_; //< 現在の環境マップ名
};
