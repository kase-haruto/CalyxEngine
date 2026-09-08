#include "SpriteAnimationAsset.h"

#include <algorithm>
#include <cmath>

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
		// 非有限値は1に戻し、整数化前に範囲を制限する。上限は縦横の積がint32_tに収まる値。
		return std::isfinite(division.x) ? static_cast<int32_t>(std::clamp(static_cast<double>(division.x), 1.0, 46340.0)) : 1;
	}

	int32_t SpriteAnimationAsset::GetDivisionY() const {
		// 横と同じ制限でゼロ除算・整数変換の未定義動作・総区画数のオーバーフローを防ぐ。
		return std::isfinite(division.y) ? static_cast<int32_t>(std::clamp(static_cast<double>(division.y), 1.0, 46340.0)) : 1;
	}

	int32_t SpriteAnimationAsset::GetFrameCapacity() const {
		// シート全体の総フレーム枠数を算出して返す
		return GetDivisionX() * GetDivisionY();
	}

} // namespace CalyxEngine
