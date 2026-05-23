#pragma once

#include <Engine/Assets/Model/ModelManager.h>
#include <Engine/Assets/Texture/TextureManager.h>
#include <Engine/Assets/DataAsset/DataAssetManager.h>

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
	 * AssetManager
	 * - アセット管理クラス
	 * - アセットの読み込み、キャッシュ管理、アセットの変更監視などを担当
	 *---------------------------------------------------------------------------------------*/
	class AssetManager {
	public:
		//===================================================================*/
		//					public function
		//===================================================================*/
		static AssetManager* GetInstance();
		/**
		 * \brief 初期化
		 */
		void Initialize(class ImGuiManager* imgui);

		/**
		 * \brief 終了処理
		 */
		void Finalize();

		// accessor ==========================//
		ModelManager*   GetModelManager() const { return modelManager_.get(); }
		TextureManager* GetTextureManager() const { return textureManager_.get(); }
		DataAssetManager* GetDataAssetManager() const { return dataAssetManager_.get(); }

	private:
		//===================================================================*/
		//					private function
		//===================================================================*/
		AssetManager() = default;


	private:
		//===================================================================*/
		//                    private members
		//===================================================================*/
		std::unique_ptr<ModelManager>   modelManager_;
		std::unique_ptr<TextureManager> textureManager_;
		std::unique_ptr<DataAssetManager> dataAssetManager_;

	};

}