#include "SpriteAnimator2d.h"

#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\System\AssetRecord.h>
#include <Engine\Objects\2D\Object2d\SpriteObject2d.h>

#include <algorithm>
#include <filesystem>
#include <cmath>
#include <limits>
#include <Engine/Objects/3D/Actor/BaseGameObject.h>

namespace CalyxEngine {
	namespace {
		std::string ResolveTexturePath(const SpriteAnimationAsset& asset) {
			// GUIDを優先してAsset移動へ追従し、未登録Assetだけ旧形式のPathへFallbackする。
			if(asset.textureGuid.isValid()) {
				if(const AssetRecord* record = AssetDatabase::GetInstance()->Get(asset.textureGuid)) {
					const auto rel = std::filesystem::relative(record->sourcePath, AssetDatabase::GetInstance()->GetRoot());
					return rel.generic_string();
				}
			}
			return asset.texturePath;
		}
	}

	void SpriteAnimator2d::Bind(SpriteObject2d* target) {
		// AnimatorはSpriteを所有せず、Binding直後に現在Frameの表示状態だけを同期する。
		target_ = target;
		modelTarget_ = nullptr;
		appliedModel_ = nullptr;
		if(target_) {
			ApplyTexture();
			ApplyFrame(currentFrame_);
		}
	}

	void SpriteAnimator2d::BindModel(BaseGameObject* target) {
		// 同じ対象とモデルなら同期済みなので、毎回の取得で画像を読み込み直さない。
		if(modelTarget_ == target && (!target || appliedModel_ == target->GetModel())) return;
		target_ = nullptr;
		modelTarget_ = target;
		ApplyTexture();
		ApplyFrame(currentFrame_);
	}

	bool SpriteAnimator2d::SetTextureSheet(const std::string& path, int32_t columns, int32_t rows) {
		// 不正な設定で既存の再生を失わないよう、アセットを作る前に範囲を検証する。
		if(path.empty() || columns < 1 || rows < 1 || columns > 46340 || rows > 46340 ||
			static_cast<int64_t>(columns) * rows > std::numeric_limits<int32_t>::max()) return false;
		auto sheet = std::make_shared<SpriteAnimationAsset>();
		sheet->texturePath = path;
		sheet->division = {static_cast<float>(columns), static_cast<float>(rows)};
		sheet->clips.clear();
		SetAnimationAsset(std::move(sheet));
		return true;
	}

	bool SpriteAnimator2d::PlayFrames(int32_t start, int32_t count, float duration, bool loop) {
		if(!asset_ || start < 0 || count < 1 ||
			static_cast<int64_t>(start) + count > asset_->GetFrameCapacity() ||
			!std::isfinite(duration) || duration <= 0.0f) return false;
		// 共有アセットを直接変更すると別オブジェクトにも影響するため、実行時クリップ用に複製する。
		auto sheet = std::make_shared<SpriteAnimationAsset>();
		sheet->textureGuid = asset_->textureGuid;
		sheet->texturePath = asset_->texturePath;
		sheet->division = asset_->division;
		sheet->clips = asset_->clips;
		const std::string name = "__runtime_frames";
		// 前回の実行時クリップだけを置き換え、名前付きクリップは引き続き利用可能にする。
		std::erase_if(sheet->clips, [&](const auto& clip) { return clip.name == name; });
		sheet->clips.push_back({name, start, count, duration, loop});
		SetAnimationAsset(std::move(sheet));
		ClearLoopOverride();
		return Play(name);
	}

	void SpriteAnimator2d::ShowFrame(int32_t frame) {
		// 固定表示へ切り替える際に残り時間と終了通知を捨て、次の更新でも区画を維持する。
		Stop();
		finished_ = false;
		frameTime_ = 0.0f;
		ApplyFrame(frame);
	}

	void SpriteAnimator2d::SetTexture(const std::string& path) {
		if(!SetTextureSheet(path, 1, 1)) return;
		if(target_) {
			target_->SetUvTranslate({0, 0});
			target_->SetUvRotate(0);
		}
	}

	void SpriteAnimator2d::SetAnimationAsset(std::shared_ptr<SpriteAnimationAsset> asset) {
		// Asset差し替え時は旧Clipの時刻・終了状態を持ち越さず、先頭Frameから選択可能にする。
		asset_ = std::move(asset);
		currentClipName_.clear();
		currentFrame_ = 0;
		frameTime_ = 0.0f;
		playing_ = false;
		finished_ = false;
		ApplyTexture();
		ApplyFrame(currentFrame_);
	}

	bool SpriteAnimator2d::Play(const std::string& clipName, bool restart) {
		if(!asset_) return false;
		const SpriteAnimationClip* clip = asset_->FindClip(clipName);
		if(!clip) return false;
		if(clip->startFrame < 0 || clip->frameCount < 1 ||
			static_cast<int64_t>(clip->startFrame) + clip->frameCount > asset_->GetFrameCapacity() ||
			!std::isfinite(clip->frameDuration) || clip->frameDuration <= 0.0f) return false;

		// Clip切替または明示Restart時だけFrameを初期化し、同一Clipの継続再生を維持する。
		if(currentClipName_ != clipName || restart) {
			currentClipName_ = clipName;
			const int32_t frameCount = std::max(1, clip->frameCount);
			// 逆再生はClip終端Frameを開始点にし、通常再生と同じ範囲定義を共有する。
			if(reversed_) {
				currentFrame_ = clip->startFrame + frameCount - 1;
			} else {
				currentFrame_ = clip->startFrame;
			}
			frameTime_ = 0.0f;
			ApplyTexture();
			ApplyFrame(currentFrame_);
		}

		playing_ = true;
		finished_ = false;
		return true;
	}

	void SpriteAnimator2d::Stop() {
		playing_ = false;
	}

	void SpriteAnimator2d::Reset() {
		const SpriteAnimationClip* clip = GetCurrentClip();
		if(clip) {
			const int32_t frameCount = std::max(1, clip->frameCount);
			currentFrame_ = reversed_ ? (clip->startFrame + frameCount - 1) : clip->startFrame;
		} else {
			currentFrame_ = 0;
		}
		frameTime_ = 0.0f;
		playing_ = false;
		finished_ = false;
		ApplyFrame(currentFrame_);
	}

	void SpriteAnimator2d::Update(float dt) {
		// 遅延ロードやモデル差し替え時は、停止中でも画像と現在区画を新しいモデルへ反映する。
		if(modelTarget_ && appliedModel_ != modelTarget_->GetModel()) {
			ApplyTexture();
			ApplyFrame(currentFrame_);
		}
		if(!playing_ || !asset_ || !std::isfinite(dt) || dt <= 0.0f) return;

		const SpriteAnimationClip* clip = GetCurrentClip();
		if(!clip || clip->frameDuration <= 0.0f) return;

		if(clip->startFrame < 0 || clip->frameCount < 1 ||
			static_cast<int64_t>(clip->startFrame) + clip->frameCount > asset_->GetFrameCapacity() ||
			!std::isfinite(clip->frameDuration)) { Stop(); return; }
		const double elapsed = static_cast<double>(frameTime_) + dt;
		// 進む区画数と端数時間を一度で求める。大きなdtでも区画数分のループは回さない。
		const double steps = std::floor(elapsed / clip->frameDuration);
		frameTime_ = static_cast<float>(std::fmod(elapsed, clip->frameDuration));
		if(steps < 1.0) return;
		const int32_t count = clip->frameCount;
		const int32_t local = std::clamp(currentFrame_ - clip->startFrame, 0, count - 1);
		const int32_t remaining = reversed_ ? local + 1 : count - local;
		// 単発再生は最後の区画を1区画分の時間だけ表示してから終了し、その区画を保持する。
		if(!ShouldLoop(*clip) && steps >= remaining) {
			currentFrame_ = clip->startFrame + (reversed_ ? 0 : count - 1);
			playing_ = false;
			finished_ = true;
			frameTime_ = 0.0f;
		} else {
			// 周回数は剰余で除去する。逆再生でも負の区画番号にならないよう総区画数を加える。
			const int64_t advance = static_cast<int64_t>(std::fmod(steps, count));
			currentFrame_ = clip->startFrame + static_cast<int32_t>(
				(local + (reversed_ ? -advance : advance) + count) % count);
		}
		ApplyFrame(currentFrame_);
	}

	void SpriteAnimator2d::SetLoopOverride(bool loop) {
		useLoopOverride_ = true;
		loopOverride_ = loop;
	}

	void SpriteAnimator2d::ClearLoopOverride() {
		useLoopOverride_ = false;
	}

	void SpriteAnimator2d::ApplyFrame(int32_t frame) {
		if(!asset_) return;

		const int32_t divX = asset_->GetDivisionX();
		const int32_t divY = asset_->GetDivisionY();
		// 不正な分割数でも除算や範囲計算を破綻させず、有効なTexture CellへClampする。
		const int32_t capacity = std::max(1, divX * divY);
		currentFrame_ = std::clamp(frame, 0, capacity - 1);

		// Frame番号をTexture Grid上の座標へ変換し、UV ScaleとOffsetへ反映する。
		const int32_t fx = currentFrame_ % divX;
		const int32_t fy = currentFrame_ / divX;
		const float frameW = 1.0f / static_cast<float>(divX);
		const float frameH = 1.0f / static_cast<float>(divY);

		if(target_) {
			target_->SetUvScale({frameW, frameH});
			target_->SetUvOffset({fx * frameW, fy * frameH});
		}
		if(modelTarget_ && modelTarget_->GetModel()) {
			// モデルはSprite専用uvOffsetを持たないため、同じ区画位置をUV平行移動へ設定する。
			// 回転が残ると隣接区画を参照するので、連番再生中は回転を0に揃える。
			auto& uv = modelTarget_->GetModel()->uvTransform;
			uv.scale = {frameW, frameH};
			uv.translate = {fx * frameW, fy * frameH};
			uv.rotate = 0.0f;
		}
	}

	void SpriteAnimator2d::ApplyTexture() {
		if(!asset_) return;
		appliedModel_ = modelTarget_ ? modelTarget_->GetModel() : nullptr;
		const std::string texturePath = ResolveTexturePath(*asset_);
		if(!texturePath.empty()) {
			if(target_) target_->SetTexture(texturePath);
			if(modelTarget_ && modelTarget_->GetModel()) modelTarget_->SetTexture(texturePath);
		}
	}

	const SpriteAnimationClip* SpriteAnimator2d::GetCurrentClip() const {
		if(!asset_ || currentClipName_.empty()) return nullptr;
		return asset_->FindClip(currentClipName_);
	}

	bool SpriteAnimator2d::ShouldLoop(const SpriteAnimationClip& clip) const {
		// Object単位Overrideが指定されていない場合はAsset側のClip設定を尊重する。
		return useLoopOverride_ ? loopOverride_ : clip.loop;
	}

} // namespace CalyxEngine
