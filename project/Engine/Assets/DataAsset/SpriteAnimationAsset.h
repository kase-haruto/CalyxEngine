#pragma once
#include <Engine/Foundation/Export/CalyxAPI.h>

#include "DataAsset.h"

#include <Engine\Foundation\Math\Vector2.h>

#include <cstdint>
#include <string>
#include <vector>

namespace CalyxEngine {

	/**
	 * @brief スプライトアニメーションの個別再生クリップの定義構造体
	 */
	struct SpriteAnimationClip {
		std::string name = "New Clip";      //< クリップ名（例: "Idle", "Walk"）
		int32_t startFrame = 0;             //< アニメーション再生開始時のフレームインデックス (0始まり)
		int32_t frameCount = 1;             //< このクリップが含む総フレーム数
		float frameDuration = 0.1f;         //< 1フレームあたりの再生時間（秒単位）
		bool loop = true;                   //< ループ再生の有無フラグ
	};

	/**
	 * @brief 2Dスプライトシート分割アニメーションパラメータを保持するデータアセットクラス
	 */
	class CALYX_API SpriteAnimationAsset : public DataAsset {
	public:
		/**
		 * @brief コンストラクタ。デフォルト設定とフィールド登録を行います。
		 */
		SpriteAnimationAsset();

		/**
		 * @brief デストラクタ
		 */
		~SpriteAnimationAsset() override = default;

		/**
		 * @brief アセットタイプ名を取得
		 * @return "SpriteAnimationAsset"
		 */
		std::string GetAssetTypeName() const override { return "SpriteAnimationAsset"; }

		Guid textureGuid;                       //< 使用するスプライトシート画像（Texture）のアセットGUID
		std::string texturePath;                //< テクスチャのソースパス（JSON保存用キャッシュ）
		Vector2 division = {1.0f, 1.0f};        //< スプライトシートの縦横分割数 (x=横列数, y=縦行数)
		std::vector<SpriteAnimationClip> clips; //< アセットに紐付くアニメーションクリップの配列

		/**
		 * @brief 名前を指定してアニメーションクリップを検索
		 * @param name クリップ名
		 * @return 見つかったクリップのポインタ。存在しない場合はnullptr
		 */
		const SpriteAnimationClip* FindClip(const std::string& name) const;

		/**
		 * @brief 横方向の分割数を整数で取得（最低 1 を保証）
		 */
		int32_t GetDivisionX() const;

		/**
		 * @brief 縦方向の分割数を整数で取得（最低 1 を保証）
		 */
		int32_t GetDivisionY() const;

		/**
		 * @brief スプライトシート全体の総最大フレーム枠数を取得 (DivisionX * DivisionY)
		 */
		int32_t GetFrameCapacity() const;

	private:
		/**
		 * @brief フィールド登録関数。シリアライズ対象として各メンバ変数を親クラスに紐付けます。
		 */
		void RegisterFields();
	};

} // namespace CalyxEngine
