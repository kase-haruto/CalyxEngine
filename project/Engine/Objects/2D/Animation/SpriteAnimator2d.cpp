#include "SpriteAnimator2d.h"

#include <Engine\Assets\Database\AssetDatabase.h>
#include <Engine\Assets\System\AssetRecord.h>
#include <Engine\Objects\2D\Object2d\SpriteObject2d.h>

#include <algorithm>
#include <filesystem>

namespace CalyxEngine {
	namespace {
		std::string ResolveTexturePath(const SpriteAnimationAsset& asset) {
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
		target_ = target;
		if(target_) {
			ApplyTexture();
			ApplyFrame(currentFrame_);
		}
	}

	void SpriteAnimator2d::SetAnimationAsset(std::shared_ptr<SpriteAnimationAsset> asset) {
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

		if(currentClipName_ != clipName || restart) {
			currentClipName_ = clipName;
			const int32_t frameCount = std::max(1, clip->frameCount);
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
		if(!playing_ || !asset_ || !target_) return;

		const SpriteAnimationClip* clip = GetCurrentClip();
		if(!clip || clip->frameDuration <= 0.0f) return;

		frameTime_ += dt;
		while(frameTime_ >= clip->frameDuration && playing_) {
			frameTime_ -= clip->frameDuration;

			const int32_t frameCount = std::max(1, clip->frameCount);
			const int32_t startFrame = clip->startFrame;
			const int32_t endFrame = startFrame + frameCount;

			if(reversed_) {
				currentFrame_--;
				if(currentFrame_ < startFrame) {
					if(ShouldLoop(*clip)) {
						currentFrame_ = endFrame - 1;
					} else {
						currentFrame_ = startFrame;
						playing_ = false;
						finished_ = true;
					}
				}
			} else {
				currentFrame_++;
				if(currentFrame_ >= endFrame) {
					if(ShouldLoop(*clip)) {
						currentFrame_ = startFrame;
					} else {
						currentFrame_ = endFrame - 1;
						playing_ = false;
						finished_ = true;
					}
				}
			}

			ApplyFrame(currentFrame_);
		}
	}

	void SpriteAnimator2d::SetLoopOverride(bool loop) {
		useLoopOverride_ = true;
		loopOverride_ = loop;
	}

	void SpriteAnimator2d::ClearLoopOverride() {
		useLoopOverride_ = false;
	}

	void SpriteAnimator2d::ApplyFrame(int32_t frame) {
		if(!asset_ || !target_) return;

		const int32_t divX = asset_->GetDivisionX();
		const int32_t divY = asset_->GetDivisionY();
		const int32_t capacity = std::max(1, divX * divY);
		currentFrame_ = std::clamp(frame, 0, capacity - 1);

		const int32_t fx = currentFrame_ % divX;
		const int32_t fy = currentFrame_ / divX;
		const float frameW = 1.0f / static_cast<float>(divX);
		const float frameH = 1.0f / static_cast<float>(divY);

		target_->SetUvScale({frameW, frameH});
		target_->SetUvOffset({fx * frameW, fy * frameH});
	}

	void SpriteAnimator2d::ApplyTexture() {
		if(!asset_ || !target_) return;
		const std::string texturePath = ResolveTexturePath(*asset_);
		if(!texturePath.empty()) {
			target_->SetTexture(texturePath);
		}
	}

	const SpriteAnimationClip* SpriteAnimator2d::GetCurrentClip() const {
		if(!asset_ || currentClipName_.empty()) return nullptr;
		return asset_->FindClip(currentClipName_);
	}

	bool SpriteAnimator2d::ShouldLoop(const SpriteAnimationClip& clip) const {
		return useLoopOverride_ ? loopOverride_ : clip.loop;
	}

} // namespace CalyxEngine
