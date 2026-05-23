#pragma once
#include "AssetChange.h"

namespace CalyxEngine {

	/*-----------------------------------------------------------------------------------------
 	* AssetChangeHandler
 	* - アセットの変更イベントを処理するクラス
 	* - AssetWatcher からのイベントを受け取り、アセットの再読み込みやキャッシュの更新などを行う
 	*---------------------------------------------------------------------------------------*/
	class IAssetChangeHandler {
	public:
		//===================================================================*/
		//                    public methods
		//===================================================================*/
		virtual ~IAssetChangeHandler() = default;

		/**
		 * \brief このハンドラが対応しているassetか判定
		 * \param type assetタイプ
		 * \return 対応していたら true
		 */
		virtual bool CanHandle(AssetChangeType& type) = 0;

		/**
		 * \brief アセットの変更イベントを処理
		 * \param event 変更イベントの内容
		 */
		virtual void OnAssetChanged(const AssetChangeEvent& event) = 0;
	};

}