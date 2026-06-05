#include "SpriteAnimationAsset.h"

#include <algorithm>

namespace CalyxEngine {

	SpriteAnimationAsset::SpriteAnimationAsset() {
		// 新規スプライトアニメーション作成時のデフォルト名
		name_ = "New Sprite Animation";
		// 最低限1つのデフォルトのクリップエントリを追加して、配列の空状態を防ぐ
		clips.push_back({});
		// シリアライズフィールド登録
		RegisterFields();
	}

	void SpriteAnimationAsset::RegisterFields() {
		// JSON保存・ロードの対象となるメンバ変数の登録
		AddField("textureGuid", textureGuid);
		AddField("division", division);
	}

	const SpriteAnimationClip* SpriteAnimationAsset::FindClip(const std::string& name) const {
		// 指定された名前と一致するアニメーションクリップを検索
		auto it = std::find_if(clips.begin(), clips.end(), [&name](const SpriteAnimationClip& clip) {
			return clip.name == name;
		});
		return it == clips.end() ? nullptr : &*it;
	}

	int32_t SpriteAnimationAsset::GetDivisionX() const {
		// 横分割数が 0 以下にならないように、最大値(1, division.x) を取得
		return std::max(1, static_cast<int32_t>(division.x));
	}

	int32_t SpriteAnimationAsset::GetDivisionY() const {
		// 縦分割数が 0 以下にならないように、最大値(1, division.y) を取得
		return std::max(1, static_cast<int32_t>(division.y));
	}

	int32_t SpriteAnimationAsset::GetFrameCapacity() const {
		// シート全体の総フレーム枠数を算出して返す
		return GetDivisionX() * GetDivisionY();
	}

} // namespace CalyxEngine
