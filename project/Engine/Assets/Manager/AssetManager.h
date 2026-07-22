#pragma once

#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Assets/DataAsset/DataAssetManager.h>
#include <Engine/Foundation/Export/CalyxAPI.h>
#include <Engine/Foundation/Audio/Audio.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * AssetManager
	 * - アセット管理クラス（ファサード / シングルトン）
	 * - モデル、テクスチャ、データアセット、オーディオなどの各サブマネージャーの初期化、
	 *   破棄、および外部へのアクセスインタフェースを提供します。
	 *---------------------------------------------------------------------------------------*/
	class AssetManager {
	public:
		//===================================================================*/
		//					public function
		//===================================================================*/
		
		/**
		 * \brief シングルトンインスタンスを取得
		 * \return AssetManagerのインスタンスへのポインタ
		 */
		CALYX_API static AssetManager* GetInstance();

		/**
		 * \brief 初期化処理
		 * \param imgui ImGui管理マネージャーへのポインタ（テクスチャプレビュー登録などに使用）
		 * \details 各サブマネージャー（Model, Texture, DataAsset）の生成・初期化とオーディオ初期化を行います。
		 */
		void Initialize(class ImGuiManager* imgui);

		/**
		 * \brief 終了（破棄）処理
		 * \details 保持しているすべてのマネージャーインスタンスのクリーンアップを行います。
		 */
		void Finalize();

		// accessor ==========================//
		
		/**
		 * \brief モデルマネージャーの取得
		 * \return ModelManagerポインタ
		 */
		ModelManager*     GetModelManager() const { return modelManager_.get(); }

		/**
		 * \brief テクスチャマネージャーの取得
		 * \return TextureManagerポインタ
		 */
		TextureManager*   GetTextureManager() const { return textureManager_.get(); }

		/**
		 * \brief データアセットマネージャーの取得
		 * \return DataAssetManagerポインタ
		 */
		DataAssetManager* GetDataAssetManager() const { return dataAssetManager_.get(); }

		/** \brief Audio manager used by the public game-side audio API. */
		CALYX_API Audio* GetAudioManager() const { return audioManager_.get(); }

	private:
		//===================================================================*/
		//					private function
		//===================================================================*/
		
		/**
		 * \brief デフォルトコンストラクタ（外部からの直接生成を禁止）
		 */
		AssetManager() = default;

	private:
		//===================================================================*/
		//                    private members
		//===================================================================*/
		std::unique_ptr<ModelManager>     modelManager_;     //< 3Dモデルデータのロード・管理担当
		std::unique_ptr<TextureManager>   textureManager_;   //< テクスチャ画像のロード・GPUリソース管理担当
		std::unique_ptr<DataAssetManager> dataAssetManager_; //< マテリアルやアニメーション等のカスタムアセット管理担当
		std::unique_ptr<Audio>			  audioManager_;	 //< 音声データのロード・再生担当
	};

}
