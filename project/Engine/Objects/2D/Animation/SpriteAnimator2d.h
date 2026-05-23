#pragma once

#include <Engine\Assets\DataAsset\SpriteAnimationAsset.h>

#include <cstdint>
#include <memory>
#include <string>

namespace CalyxEngine {
	class SpriteObject2d;

	class SpriteAnimator2d {
	public:
		void Bind(SpriteObject2d* target);
		void SetAnimationAsset(std::shared_ptr<SpriteAnimationAsset> asset);
		bool Play(const std::string& clipName, bool restart = true);
		void Stop();
		void Reset();
		void Update(float dt);
		void ApplyFrame(int32_t frame);

		void SetReversed(bool reversed) { reversed_ = reversed; }
		void SetLoopOverride(bool loop);
		void ClearLoopOverride();

		bool IsPlaying() const { return playing_; }
		bool IsFinished() const { return finished_; }
		const std::string& GetCurrentClipName() const { return currentClipName_; }
		int32_t GetCurrentFrame() const { return currentFrame_; }
		std::shared_ptr<SpriteAnimationAsset> GetAnimationAsset() const { return asset_; }
		bool IsReversed() const { return reversed_; }
		bool HasLoopOverride() const { return useLoopOverride_; }
		bool GetLoopOverride() const { return loopOverride_; }

	private:
		void ApplyTexture();
		const SpriteAnimationClip* GetCurrentClip() const;
		bool ShouldLoop(const SpriteAnimationClip& clip) const;

	private:
		SpriteObject2d* target_ = nullptr;
		std::shared_ptr<SpriteAnimationAsset> asset_;
		std::string currentClipName_;
		int32_t currentFrame_ = 0;
		float frameTime_ = 0.0f;
		bool playing_ = false;
		bool finished_ = false;
		bool reversed_ = false;
		bool useLoopOverride_ = false;
		bool loopOverride_ = true;
	};

} // namespace CalyxEngine
